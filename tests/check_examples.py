#!/usr/bin/env python3
"""Black-box checks for the independent examples; Python standard library only."""

import collections
import os
from pathlib import Path
import re
import signal
import subprocess
import sys
import tempfile


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def run_program(executable, *arguments, cwd=None, overrides=None):
    environment = os.environ.copy()
    environment["LC_ALL"] = "C"
    if overrides:
        environment.update(overrides)
    command = [str(executable), *arguments]
    with subprocess.Popen(
        command,
        cwd=cwd,
        env=environment,
        stdin=subprocess.DEVNULL,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        start_new_session=(os.name == "posix"),
    ) as process:
        try:
            stdout, stderr = process.communicate(timeout=8)
        except subprocess.TimeoutExpired:
            # Kill the entire example's process group, including forked children.
            if os.name == "posix":
                try:
                    os.killpg(process.pid, signal.SIGKILL)
                except ProcessLookupError:
                    pass
            else:
                process.kill()
            process.communicate()
            raise AssertionError("example did not finish within 8 seconds") from None
        return subprocess.CompletedProcess(command, process.returncode, stdout, stderr)


def text(result):
    return result.stdout.decode("utf-8", errors="replace")


def expect_success(result):
    require(
        result.returncode == 0,
        "exit code {} (expected 0)\nstderr:\n{}".format(
            result.returncode, result.stderr[-4000:].decode("utf-8", errors="replace")
        ),
    )
    require(not result.stderr, "unexpected stderr: {!r}".format(result.stderr[-4000:]))


def check_std_thread(executable):
    result = run_program(executable)
    expect_success(result)
    output = text(result)
    require(output.count("All threads finished.") == 1, "missing completion message")
    require(re.search(r"Main thread id: \S+", output), "missing main thread id")
    require(re.search(r"Hardware concurrency: \d+", output), "missing concurrency hint")
    # Worker lines can interleave; they must not be compared to a fixed transcript.


def check_pthread(executable):
    result = run_program(executable)
    expect_success(result)
    require(
        "Thread 1 returned: 100, Thread 2 returned: 200" in text(result),
        "pthread results are missing or incorrect",
    )
    require("All threads finished." in text(result), "missing completion message")


def check_thread_sync(executable):
    result = run_program(executable)
    expect_success(result)
    queue = collections.deque()
    produced = collections.Counter()
    consumed = collections.Counter()
    workers = collections.Counter()
    pattern = re.compile(r"\[(Producer|Consumer) ([12])\]: (Push|pop) (\d+), size=(\d+)")
    lines = text(result).splitlines()
    require(len(lines) == 40, "expected 20 pushes and 20 pops, got {} lines".format(len(lines)))

    for line in lines:
        match = pattern.fullmatch(line)
        require(match is not None, "malformed or interleaved queue log: {!r}".format(line))
        role, worker, operation, value, size = match.groups()
        value, size = int(value), int(size)
        workers[(role, worker)] += 1
        require(0 <= value <= 9, "unexpected queue value")
        if role == "Producer":
            require(operation == "Push", "producer performed the wrong operation")
            queue.append(value)
            produced[value] += 1
        else:
            require(operation == "pop", "consumer performed the wrong operation")
            require(queue, "consumer removed an item from an empty queue")
            require(queue.popleft() == value, "queue FIFO order was violated")
            consumed[value] += 1
        require(0 <= size <= 5, "queue exceeded its capacity")
        require(size == len(queue), "logged queue size does not match the operation history")

    expected = collections.Counter({value: 2 for value in range(10)})
    require(produced == expected and consumed == expected, "items were lost or duplicated")
    require(not queue, "queue was not drained")
    require(
        workers == collections.Counter({
            ("Producer", "1"): 10, ("Producer", "2"): 10,
            ("Consumer", "1"): 10, ("Consumer", "2"): 10,
        }),
        "unexpected per-thread work count",
    )


def check_fork(executable):
    result = run_program(executable)
    expect_success(result)
    output = text(result)
    before = re.search(r"\[before fork\] PID= (\d+)", output)
    child = re.search(r"\[Child\] PID= (\d+), Parent PID= (\d+)", output)
    parent = re.search(r"\[Parent\] child PID= (\d+)", output)
    require(before and child and parent, "missing parent/child PID output")
    require(child[1] == parent[1], "parent reported a different child PID")
    require(child[2] == before[1], "child reported a different parent PID")
    require(child[1] != before[1], "parent and child must have different PIDs")
    require(output.count("[before fork]") == 1, "pre-fork output was duplicated")
    require("[Parent] child exited with status= 0" in output, "child exit was not collected")


def check_exec(executable, mode):
    with tempfile.TemporaryDirectory(prefix="embedded-exec-", dir=executable.parent) as directory:
        root = Path(directory)
        overrides = {}
        expected_code = 0
        if mode == "exec_demo":
            (root / "visible_marker.txt").write_text("visible\n", encoding="utf-8")
            (root / ".hidden_marker").write_text("hidden\n", encoding="utf-8")
        else:
            overrides["PATH"] = str(root)
            if mode == "exec_nonzero":
                script = "#!/bin/sh\nexit 7\n"
                expected_code = 7
            elif mode == "exec_signal":
                script = "#!/bin/sh\nkill -TERM $$\n"
                expected_code = 128 + signal.SIGTERM
            else:
                script = None  # Empty PATH directory: execvp must fail.
                expected_code = 1
            if script is not None:
                replacement = root / "ls"
                replacement.write_text(script, encoding="utf-8")
                replacement.chmod(0o755)

        result = run_program(executable, cwd=root, overrides=overrides)
        require(
            result.returncode == expected_code,
            "expected exit {}, got {}; stderr={!r}".format(
                expected_code, result.returncode, result.stderr
            ),
        )
        if mode == "exec_signal":
            require(
                "terminated by signal= {}".format(int(signal.SIGTERM)) in result.stderr.decode("utf-8"),
                "signal termination was not reported",
            )
        else:
            require(
                "[Parent] child (running ls) finished with status= {}".format(expected_code)
                in text(result),
                "parent did not report the child exit status",
            )
        if mode == "exec_demo":
            expect_success(result)
            require("visible_marker.txt" in text(result), "ls did not use the working directory")
            require(".hidden_marker" in text(result), "ls did not include hidden files")
        elif mode == "exec_missing":
            require(b"execvp:" in result.stderr, "missing execvp diagnostic")
        elif mode == "exec_nonzero":
            require(not result.stderr, "unexpected error executing the replacement command")


def check_pipe(executable, mode, length=None):
    if mode == "pipe_usage":
        result = run_program(executable, "one", "two")
        require(result.returncode == 2, "extra arguments should return usage status 2")
        require(b"Usage:" in result.stderr and not result.stdout, "missing usage diagnostic")
        return
    if mode == "pipe_default":
        message = "Hello from parent process!"
        arguments = []
    elif mode == "pipe_text":
        message = "你好，管道！ spaces ; $HOME 'quotes' \\\nsecond line"
        arguments = [message]
    else:
        # Printable, nonuniform bytes exercise chunk boundaries and argv quoting.
        message = "".join(chr(33 + (index * 37 + index // 11) % 90) for index in range(length))
        arguments = [message]
    result = run_program(executable, *arguments)
    expect_success(result)
    expected = b"[Child] received: " + message.encode("utf-8") + b"\n"
    require(
        result.stdout == expected,
        "pipe payload differs: expected {} output bytes, got {}".format(
            len(expected), len(result.stdout)
        ),
    )


def main():
    require(len(sys.argv) >= 3, "usage: check_examples.py CASE EXECUTABLE [LENGTH]")
    case = sys.argv[1]
    executable = Path(sys.argv[2]).resolve()
    require(executable.is_file(), "executable does not exist: {}".format(executable))
    if case == "std_thread":
        check_std_thread(executable)
    elif case == "pthread_demo":
        check_pthread(executable)
    elif case == "thread_sync":
        check_thread_sync(executable)
    elif case == "fork_demo":
        check_fork(executable)
    elif case in ("exec_demo", "exec_missing", "exec_nonzero", "exec_signal"):
        check_exec(executable, case)
    elif case in ("pipe_default", "pipe_text", "pipe_usage", "pipe_length"):
        length = int(sys.argv[3]) if case == "pipe_length" else None
        check_pipe(executable, case, length)
    else:
        raise AssertionError("unknown test case: {}".format(case))
    print("PASS: {}".format(case))


if __name__ == "__main__":
    try:
        main()
    except (AssertionError, OSError, ValueError) as error:
        print("FAIL: {}".format(error), file=sys.stderr)
        sys.exit(1)
