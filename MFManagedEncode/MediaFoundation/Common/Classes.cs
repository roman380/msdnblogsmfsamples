// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

namespace MFManagedEncode.MediaFoundation
{
    using System;
    using System.Text;
    using MFManagedEncode.MediaFoundation.Com;
    using MFManagedEncode.MediaFoundation.Com.Structures;

    /// <summary>
    ///     Two uint number packed inside an ulong data type.
    /// </summary>
    internal class PackedINT32
    {
        protected uint high;
        protected uint low;

        public PackedINT32()
        {
            this.High = 0;
            this.Low = 0;
        }

        public PackedINT32(uint high, uint low)
        {
            this.High = high;
            this.Low = low;
        }

        public uint High
        {
            get
            {
                return this.high;
            }

            set
            {
                this.high = value;
            }
        }

        public uint Low
        {
            get
            {
                return this.low;
            }

            set
            {
                this.low = value;
            }
        }

        public ulong Packed
        {
            get
            {
                return (((ulong)this.High) << 32) | ((ulong)this.Low);
            }

            set
            {
                this.High = (uint)(value >> 32);
                this.Low = (uint)(value & 0xFFFFFFFF);
            }
        }

        public override string ToString()
        {
            return "H:" + 
                this.High.ToString(System.Globalization.CultureInfo.InvariantCulture) + 
                " L:" + 
                this.Low.ToString(System.Globalization.CultureInfo.InvariantCulture);
        }
    }

    /// <summary>
    ///     Video size packed inside an ulong data type.
    /// </summary>
    internal class PackedSize
        : PackedINT32
    {
        public PackedSize()
        {
            this.high = 0;
            this.low = 0;
        }

        public PackedSize(uint width, uint height)
        {
            this.high = width;
            this.low = height;
        }

        public override string ToString()
        {
            return this.High.ToString(System.Globalization.CultureInfo.InvariantCulture) + 
                " x " + 
                this.Low.ToString(System.Globalization.CultureInfo.InvariantCulture);
        }
    }

    /// <summary>
    ///     Audio format settings.
    /// </summary>
    internal class AudioFormat
    {
        private Guid subtype;
        private uint avgBytePerSecond;
        private uint numOfChannels;
        private uint samplesPerSecond;
        private uint bitsPerSample;
        private uint blockAlignment;

        public AudioFormat(string subtype)
        {
            this.Subtype = new Guid(subtype);
            this.BlockAlignment = 2973;
            this.SamplesPerSecond = 44100;
            this.AvgBytePerSecond = 16005;
            this.NumOfChannels = 2;
            this.BitsPerSample = 16;
        }

        public Guid Subtype
        {
            get
            {
                return this.subtype;
            }

            set
            {
                this.subtype = value;
            }
        }

        public uint AvgBytePerSecond
        {
            get
            {
                return this.avgBytePerSecond;
            }

            set
            {
                this.avgBytePerSecond = value;
            }
        }

        public uint NumOfChannels
        {
            get
            {
                return this.numOfChannels;
            }

            set
            {
                this.numOfChannels = value;
            }
        }

        public uint SamplesPerSecond
        {
            get
            {
                return this.samplesPerSecond;
            }

            set
            {
                this.samplesPerSecond = value;
            }
        }

        public uint BitsPerSample
        {
            get
            {
                return this.bitsPerSample;
            }

            set
            {
                this.bitsPerSample = value;
            }
        }

        public uint BlockAlignment
        {
            get
            {
                return this.blockAlignment;
            }

            set
            {
                this.blockAlignment = value;
            }
        }

        public override string ToString()
        {
            StringBuilder result = new StringBuilder("Subtype = ");
            result.AppendLine(this.Subtype.ToString());
            result.Append("BlockAlignment = ");
            result.AppendLine(this.BlockAlignment.ToString(System.Globalization.CultureInfo.InvariantCulture));
            result.Append("SamplesPerSecond = ");
            result.AppendLine(this.SamplesPerSecond.ToString(System.Globalization.CultureInfo.InvariantCulture));
            result.Append("AvgBytePerSecond = ");
            result.AppendLine(this.AvgBytePerSecond.ToString(System.Globalization.CultureInfo.InvariantCulture));
            result.Append("NumOfChannels = ");
            result.AppendLine(this.NumOfChannels.ToString(System.Globalization.CultureInfo.InvariantCulture));
            result.Append("BitsPerSample = ");
            result.AppendLine(this.BitsPerSample.ToString(System.Globalization.CultureInfo.InvariantCulture));

            return result.ToString();
        }
    }

    /// <summary>
    ///     Video format settings.
    /// </summary>
    internal class VideoFormat
    {
        private Guid subtype;
        private PackedSize frameSize;
        private PackedINT32 frameRate;
        private PackedINT32 pixelAspectRatio;
        private uint avgBitRate;
        private uint interlaceMode;

        public VideoFormat(string subtype)
        {
            this.Subtype = new Guid(subtype);
            this.FrameSize = new PackedSize(320, 240);
            this.FrameRate = new PackedINT32(30,1);
            this.PixelAspectRatio = new PackedINT32(1, 1);
            this.AvgBitRate = 1000000;
            this.InterlaceMode = 2;
        }

        public Guid Subtype
        {
            get
            {
                return this.subtype;
            }

            set
            {
                this.subtype = value;
            }
        }

        public PackedSize FrameSize
        {
            get
            {
                return this.frameSize;
            }

            set
            {
                this.frameSize = value;
            }
        }

        public PackedINT32 FrameRate
        {
            get
            {
                return this.frameRate;
            }

            set
            {
                this.frameRate = value;
            }
        }

        public PackedINT32 PixelAspectRatio
        {
            get
            {
                return this.pixelAspectRatio;
            }

            set
            {
                this.pixelAspectRatio = value;
            }
        }

        public uint AvgBitRate
        {
            get
            {
                return this.avgBitRate;
            }

            set
            {
                this.avgBitRate = value;
            }
        }

        public uint InterlaceMode
        {
            get
            {
                return this.interlaceMode;
            }

            set
            {
                this.interlaceMode = value;
            }
        }

        public override string ToString()
        {
            StringBuilder result = new StringBuilder("Subtype = ");
            result.AppendLine(this.Subtype.ToString());
            result.Append("FrameSize = ");
            result.AppendLine(this.FrameSize.ToString());
            result.Append("FrameRate = ");
            result.AppendLine(this.FrameRate.ToString());
            result.Append("PixelAspectRatio = ");
            result.AppendLine(this.PixelAspectRatio.ToString());
            result.Append("AvgBitRate = ");
            result.AppendLine(this.AvgBitRate.ToString(System.Globalization.CultureInfo.InvariantCulture));
            result.Append("InterlaceMode = ");
            result.AppendLine(this.InterlaceMode.ToString(System.Globalization.CultureInfo.InvariantCulture));

            return result.ToString();
        }
    }
}
