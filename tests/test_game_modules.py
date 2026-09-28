#!/usr/bin/env python3
"""Test actual dynamic discovery/selection with two independent fixture modules."""
import argparse
from pathlib import Path
import shutil
import subprocess
import tempfile


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('binary',type=Path)
    parser.add_argument('one',type=Path)
    parser.add_argument('two',type=Path)
    parser.add_argument('bad',type=Path)
    args=parser.parse_args()
    binary=args.binary.resolve()
    with tempfile.TemporaryDirectory(prefix='apex-modules-') as temp:
        root=Path(temp); modules=root/'plugins'; modules.mkdir()
        one=modules/args.one.name; two=modules/args.two.name
        shutil.copy2(args.one,one); shutil.copy2(args.two,two)
        def run(*arguments,success=True):
            result=subprocess.run([str(binary),*map(str,arguments)],cwd=root,capture_output=True,text=True)
            assert (result.returncode==0)==success,(result.returncode,result.stdout,result.stderr)
            return result.stdout+result.stderr
        listing=run('modules','--module-dir',modules)
        assert 'fixture-one' in listing and 'fixture-two' in listing and 'generation-zero' in listing and 'rage2 - Rage 2' in listing
        game=root/'game'; game.mkdir()
        marker=game/'fixture-two.game'; marker.touch()
        output=root/'output'
        # Auto selection must choose the second module, independent of enumeration order.
        run('extract',game,'asset.test','--module-dir',modules,'-o',output)
        assert (output/'fixture-two.txt').read_text().splitlines()==['fixture-two','asset.test','0']
        run('extract',game,'raw.test','--module',two,'-r','-o',output)
        assert (output/'fixture-two.txt').read_text().splitlines()==['fixture-two','raw.test','1']
        message=run('extract',game,'throw','--module',two,success=False)
        assert 'Fixture extract exception' in message
        message=run('search','cpp-vector','--module',two)
        assert 'cpp-vector' in message and 'x'*4096 in message
        message=run('extract',game,'asset','--module-dir',modules,'--module','fixture-one',success=False)
        assert 'does not support this root' in message
        (game/'fixture-one.game').touch()
        message=run('extract',game,'asset','--module-dir',modules,success=False)
        assert 'Multiple modules support' in message
        run('extract',game,'chosen','--module-dir',modules,'--module','fixture-one','-o',output)
        assert (output/'fixture-one.txt').read_text().splitlines()[1]=='chosen'
        message=run('extract-anims',game,'skeleton','animation','--module',one,'-o',output,success=False)
        assert 'does not support animation' in message
        (game/'fixture-one.game').unlink(); marker.unlink()
        message=run('extract',game,'asset','--module-dir',modules,success=False)
        assert 'No installed game module supports' in message
        message=run('modules','--module',args.bad.resolve(),success=False)
        assert 'Incompatible game module ABI' in message
        shutil.copy2(args.one,modules/('duplicate'+args.one.suffix))
        message=run('modules','--module-dir',modules,success=False)
        assert 'Duplicate module id' in message
        message=run('modules','--module',root/'missing.so',success=False)
        assert 'Cannot load' in message
    print('Dynamic game module tests passed')


if __name__=='__main__':
    main()
