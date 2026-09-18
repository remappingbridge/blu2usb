"""Inject the original synchronous-start bug into a scratch copy, never shipped code."""
import pathlib, resource, subprocess, sys, tempfile
root = pathlib.Path(sys.argv[1]).resolve()
bt = pathlib.Path(sys.argv[2]).resolve()
resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
source = (root / 'src/classic_hid/classic_hid_pico.c').read_text()
needle = 'btstack_run_loop_execute_on_main_thread(&g_deferred_start);'
assert source.count(needle) == 1
with tempfile.TemporaryDirectory() as directory:
    temporary = pathlib.Path(directory)
    mutant = temporary / 'mutant.c'
    mutant.write_text(source.replace(needle, 'start_hid_after_bonding(NULL);'))
    executable = temporary / 'negative-control'
    command = ['cc', '-std=c11', '-UNDEBUG', '-DENABLE_CLASSIC=1', '-DENABLE_BLE=1',
               '-ffunction-sections', '-fdata-sections', '-Wl,--gc-sections']
    for include in [root/'include', bt/'src', bt/'3rd-party/bluedroid/encoder/include',
                    bt/'3rd-party/bluedroid/decoder/include', bt/'3rd-party/yxml']:
        command += ['-I', str(include)]
    for file in [root/'tests/test_g07_classic_adapter.c', mutant,
                 root/'src/classic_hid/classic_hid_parser.c',
                 root/'src/keyboard_transport/keyboard_transport.c',
                 root/'src/bt_runtime/bt_runtime.c',
                 bt/'src/btstack_hid_parser.c', bt/'src/btstack_util.c']:
        command.append(str(file))
    subprocess.run(command + ['-o', str(executable)], check=True)
    result = subprocess.run([str(executable), 'success'], capture_output=True, text=True)
    assert result.returncode != 0 and '!old_acl_present' in result.stderr, result.stderr
print('PASS: restoring synchronous HID startup fails the stale-ACL invariant')
