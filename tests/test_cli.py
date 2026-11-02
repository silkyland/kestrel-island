import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile
import unittest

APP = Path(sys.argv.pop(1)).resolve()
ROOT = Path(__file__).resolve().parents[1]

class CliTests(unittest.TestCase):
    def test_disconnected_bus_is_failure(self):
        env = dict(os.environ, DBUS_SESSION_BUS_ADDRESS="unix:path=/tmp/kestrel-nonexistent-test-bus", QT_QPA_PLATFORM="offscreen")
        result = subprocess.run([str(APP)], env=env, capture_output=True, timeout=5)
        self.assertEqual(result.returncode, 2)

    def test_task_file_is_bounded(self):
        with tempfile.NamedTemporaryFile() as file:
            file.write(b"x" * 17000); file.flush()
            env = dict(os.environ, QT_QPA_PLATFORM="offscreen")
            result = subprocess.run([str(APP), "--task-event-file", file.name], env=env, capture_output=True, timeout=5)
            self.assertEqual(result.returncode, 2)

    def test_wrapper_preserves_output_and_exit(self):
        with tempfile.TemporaryDirectory() as directory:
            fake = Path(directory) / "kestrel-fake"
            log = Path(directory) / "events"
            fake.write_text("#!/usr/bin/env python3\nimport json,sys,os\nwith open(os.environ['TEST_TASK_LOG'],'a') as f:f.write(open(sys.argv[2]).read()+'\\n')\n")
            fake.chmod(0o700)
            env = dict(os.environ, KESTREL_BINARY=str(fake), TEST_TASK_LOG=str(log))
            result = subprocess.run([sys.executable, str(ROOT / "scripts/kestrel-task"), "--label", "Unit build", "--", sys.executable, "-c", "print('output preserved'); raise SystemExit(7)"], env=env, text=True, capture_output=True, timeout=10)
            self.assertEqual(result.returncode, 7)
            self.assertIn("output preserved", result.stdout)
            events = [json.loads(line) for line in log.read_text().splitlines()]
            self.assertEqual([event['state'] for event in events], ['running', 'failed'])
            self.assertEqual(events[0]['id'], events[1]['id'])

    def test_wrapper_preserves_signal(self):
        result = subprocess.run([sys.executable, str(ROOT / "scripts/kestrel-task"), "--", sys.executable, "-c", "import os,signal;os.kill(os.getpid(),signal.SIGTERM)"], env=dict(os.environ, KESTREL_BINARY="/nonexistent/kestrel"), capture_output=True, timeout=10)
        self.assertEqual(result.returncode, -signal.SIGTERM)

unittest.main()
