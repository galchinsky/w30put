import os
import glob
import librosa
import numpy as np
import argparse

def main():
    parser = argparse.ArgumentParser(description='Init W30 disk image and load wavs')
    parser.add_argument('image_name', help='')
    parser.add_argument('files', nargs='+',
                        help='files to load. '
                        'They must be handled by librosa')
    parser.add_argument('--load_sr', 
                        type=int,
                        default=15000,
                        help='resample input wav to this samplerate. '
                        'This can be used to save memory')
    parser.add_argument('--store_sr',
                        type=int,
                        default=15000,
                        choices=[15000, 3000],
                        help='what samplerate to tell to W30, '
                        '15000 or 30000')
    parser.add_argument('--chunks',
                        type=int,
                        default=9999,
                        help='W-30 stores audio into 12288 byte sized chunks'
                        'So if you want to cut samples to get more memory'
                        'it makes sense to cut to a multiple of chunks')
    parser.add_argument('--runhxc', dest='runhxc', action='store_true')
    parser.set_defaults(runhxc=False)
    args = parser.parse_args()

    os.system('./winit %s.img' % args.image_name)

    for fname in args.files:
        basename = os.path.basename(fname)
        soundname, _ = os.path.splitext(basename)
        data, fs = librosa.load(fname, sr=args.load_sr, mono=True)
        if len(data) > 12288*args.chunks:
            data=data[:(12288*args.chunks)]
        (data * 32767).astype(np.int16).tofile(soundname)
        line = './wput %d %s.img %s' % (args.store_sr, args.image_name, soundname)
        os.system(line)
        os.system('rm ' + soundname)

    if args.runhxc:
        os.system('wine ./hxcfe.exe -finput:%s.img -foutput:%s.hfe -conv:HXC_HFE' % (args.image_name, args.image_name))

if __name__== "__main__":
 main()
