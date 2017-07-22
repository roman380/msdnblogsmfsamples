// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

namespace MFManagedEncode.Gui
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Windows;
    using System.Windows.Controls;
    using System.Windows.Input;
    using System.Windows.Media;
    using System.Windows.Media.Animation;
    using System.Windows.Threading;
    using MFManagedEncode.MediaFoundation;
    using MFManagedEncode.MediaFoundation.Com;
    using Microsoft.Win32;

    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        private const int MAX_RETRIES = 10;

        private Storyboard mediaStoryboard;
        private MediaTimeline mediaTimeline;
        private int mediaFailedCount;

        public MainWindow()
        {
            InitializeComponent();
        }

        private void WindowRoot_Loaded(object sender, RoutedEventArgs e)
        {
            this.mediaStoryboard = (Storyboard)windowRoot.Resources["mediaStoryboard"];
            this.mediaTimeline = (MediaTimeline)this.mediaStoryboard.Children[0];
            this.mediaFailedCount = 0;

            this.videoTrimmer.SeekerValueChanging += VideoTrimmer_SeekerValueChanging;
            this.advancedExpander.IsExpanded = false;

            // Disable controls
            this.outputText.Text = Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments);
            DisableAllControls();
        }

        private void MediaTimeline_CurrentTimeInvalidated(object sender, EventArgs e)
        {
            // Update the seeker position 
            this.videoTrimmer.SeekerPosition = this.mediaElement.Position.TotalMilliseconds;
        }

        private void MediaElement_MediaFailed(object sender, ExceptionRoutedEventArgs e)
        {
            if (this.mediaFailedCount++ < MAX_RETRIES)
            {
                // Retry playback
                OpenMedia(this.mediaTimeline.Source);
            }
            else
            {
                // Disable controls
                DisableAllControls();
            }
        }

        private void MediaElement_MouseEnter(object sender, MouseEventArgs e)
        {
            // Play the playback controls show animation
            ((Storyboard)this.windowRoot.Resources["showPlaybackControlsStoryboard"]).Begin();
        }

        private void MediaElement_MouseLeave(object sender, MouseEventArgs e)
        {
            // Play the playback controls hide animation
            ((Storyboard)this.windowRoot.Resources["hidePlaybackControlsStoryboard"]).Begin();
        }

        private void MediaElement_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            // If the playback control is hidden return
            if (this.mediaControlBack.Visibility == Visibility.Hidden)
            {
                return;
            }

            if (this.mediaControlPlay.Visibility == Visibility.Visible)
            {
                // If the play control is visible play the media
                PlayMedia();
            }
            else
            {
                // Otherwise pause the media
                PauseMedia();
            }
        }

        private void MediaElement_MediaEnded(object sender, RoutedEventArgs e)
        {
            // Rewind the media
            PauseMedia();
            this.mediaStoryboard.Seek(new TimeSpan(0));
            this.videoTrimmer.SeekerPosition = 0;
        }

        private void MediaElement_MediaOpened(object sender, RoutedEventArgs e)
        {
            // Set the trimmer limits
            this.videoTrimmer.MinValue = 0;
            this.videoTrimmer.MaxValue = mediaElement.NaturalDuration.TimeSpan.TotalMilliseconds;

            // Enable the required controls
            this.mediaFormat.IsEnabled = true;
            this.outputText.IsEnabled = true;
            this.browseOutput.IsEnabled = true;
            this.videoTrimmer.IsEnabled = true;
            this.continueButton.IsEnabled = true;
            this.videoSize.IsEnabled = mediaElement.HasVideo;
            this.videoCodec.IsEnabled = mediaElement.HasVideo;
            this.videoQuality.IsEnabled = mediaElement.HasVideo;
            this.audioQuality.IsEnabled = mediaElement.HasAudio;
            this.audioCodec.IsEnabled = mediaElement.HasAudio;
            this.mediaControlBack.Visibility = Visibility.Visible;
            this.mediaControlPlay.Visibility = Visibility.Visible;

            // If the media has video show the MediaElement, otherwise show an empty rectangle
            this.mediaElementBack.Visibility = mediaElement.HasVideo ? Visibility.Hidden : Visibility.Visible;
            this.mediaElement.Visibility = mediaElement.HasVideo ? Visibility.Visible : Visibility.Hidden;

            // Set the seeker position at the start of the media
            this.videoTrimmer.SeekerPosition = 0;
            this.videoTrimmer.IsSeekerLabelAlwaysVisible = false;

            // Always show playback controls for audio files
            if (this.mediaElement.HasVideo == false)
            {
                ((Storyboard)this.windowRoot.Resources["showPlaybackControlsStoryboard"]).Begin();
            }

            // Update the source and output text boxes
            this.sourceText.Text = new System.IO.FileInfo(this.mediaTimeline.Source.OriginalString).Name;
            this.outputText.Text = new System.IO.FileInfo(this.mediaTimeline.Source.OriginalString).DirectoryName;

            // Update video size options
            UpdateVideoSizeOptions();

            // Pause the media at the start
            this.mediaStoryboard.Pause();
            this.mediaStoryboard.Seek(new TimeSpan(0));
        }

        private void BrowseSource_Click(object sender, RoutedEventArgs e)
        {
            OpenFileDialog openDialog = new OpenFileDialog();

            // Show the open file dialog
            openDialog.Title = "Open source file";
            openDialog.Filter = "Media files|*.AVI;*.MP4;*.WAV;*.3GP;*.MOV;*.3G2;*.WMV;*.ASF;*.MP3;*.WMA;*.M4A;*.AAC;|All files (*.*)|*.*;";
            if (openDialog.ShowDialog() == false)
            {
                return;
            }

            // Enable scrubbing for video files
            string fileExtension = new System.IO.FileInfo(openDialog.FileName).Extension.ToLowerInvariant();
            this.mediaElement.ScrubbingEnabled = fileExtension == ".avi" ||
                                            fileExtension == ".mp4" ||
                                            fileExtension == ".3gp" ||
                                            fileExtension == ".mov" ||
                                            fileExtension == ".mpg" ||
                                            fileExtension == ".mpeg" ||
                                            fileExtension == ".m2ts" ||
                                            fileExtension == ".ts" ||
                                            fileExtension == ".wmv" ||
                                            fileExtension == ".asf";

            // Open the media
            OpenMedia(new Uri(openDialog.FileName));
        }

        private void BrowseOutput_Click(object sender, RoutedEventArgs e)
        {
            SaveFileDialog saveFile = new SaveFileDialog();

            // Set the dialog path to the current output folder
            try
            {
                saveFile.InitialDirectory = this.outputText.Text;
            }
            catch
            {
                saveFile.InitialDirectory = System.IO.Path.GetDirectoryName(this.mediaTimeline.Source.OriginalString);
            }

            // Show the dialog
            saveFile.Title = "Select the output directory";
            saveFile.Filter = "Directory|*.*;";
            saveFile.CreatePrompt = false;
            saveFile.FileName = saveFile.InitialDirectory + "\\Select the output directory";
            saveFile.OverwritePrompt = false;
            if (saveFile.ShowDialog() != true)
            {
                return;
            }

            // Set the new output text
            this.outputText.Text = System.IO.Path.GetDirectoryName(saveFile.FileName);
        }

        private void ExitButton_Click(object sender, RoutedEventArgs e)
        {
            Close();
        }

        private void ContinueButton_Click(object sender, RoutedEventArgs e)
        {
            VideoFormat videoFormat = null;
            AudioFormat audioFormat = null;

            // Create the audio format
            if (this.audioCodec.Text == "WMA 9")
            {
                audioFormat = new AudioFormat(Consts.MFAudioFormat_WMAudioV8);
            }
            else if (this.audioCodec.Text == "AAC")
            {
                audioFormat = new AudioFormat(Consts.MFAudioFormat_AAC);
            }
            else
            {
                audioFormat = new AudioFormat(Consts.MFAudioFormat_WMAudioV9);
            }

            // Create the video format
            if (this.videoCodec.Text == "WMV 9")
            {
                videoFormat = new VideoFormat(Consts.MFVideoFormat_WMV3);
            }
            else if (this.videoCodec.Text == "WMV 8")
            {
                videoFormat = new VideoFormat(Consts.MFVideoFormat_WMV2);
            }
            else if (this.videoCodec.Text == "H.264 (AVC)")
            {
                videoFormat = new VideoFormat(Consts.MFVideoFormat_H264);
            }
            else
            {
                videoFormat = new VideoFormat(Consts.MFVideoFormat_WVC1);
            }

            // Set the bitrates
            videoFormat.AvgBitRate = uint.Parse(this.videoQuality.Text.Split(' ')[0], NumberFormatInfo.InvariantInfo) * 1000;
            audioFormat.AvgBytePerSecond = (uint.Parse(this.audioQuality.Text.Split(' ')[0], NumberFormatInfo.InvariantInfo) * 1000) / 8;

            // Set the video size
            videoFormat.FrameSize = (PackedSize)this.videoSize.SelectedItem;

            // Determinate the output extension
            string outputExtension = "wmv";

            if (this.mediaFormat.Text.StartsWith("MP4", StringComparison.Ordinal))
            {
                outputExtension = "mp4";
            }
            else if (this.mediaElement.HasVideo == false)
            {
                outputExtension = "wma";
            }

            // Create the transcode arguments 
            Dictionary<string, object> encodeArgs = new Dictionary<string, object>();

            encodeArgs.Add("InputURL", this.mediaTimeline.Source.OriginalString);
            encodeArgs.Add("OutputURL", this.outputText.Text);
            encodeArgs.Add("OutputExtension", outputExtension);

            encodeArgs.Add("StartTime", Convert.ToUInt64(this.videoTrimmer.StartPosition) * 10000);
            encodeArgs.Add("EndTime", Convert.ToUInt64(this.videoTrimmer.EndPosition) * 10000);

            encodeArgs.Add("VideoFormat", videoFormat);
            encodeArgs.Add("AudioFormat", audioFormat);

            // Hide this window
            Hide();

            // Stop playback
            this.mediaStoryboard.Stop();
            this.mediaElement.Close();

            // Launch the conversion progress window
            ConversionProgress conversionWindow = new ConversionProgress(encodeArgs, mediaFormat.Text.Contains("Sink writer"));
            conversionWindow.ShowDialog();

            // Close this window
            Close();
        }

        private void MediaFormat_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (this.audioCodec == null || this.audioCodec.Items == null || this.videoCodec == null || this.videoCodec.Items == null)
            {
                return;
            }

            // Set the codec options for the selected media format
            if (((ComboBoxItem)this.mediaFormat.SelectedItem).Content.ToString().StartsWith("MP4", StringComparison.Ordinal) == true)
            {
                SetMPEG4CodecsOptions();
            }
            else
            {
                SetWindowsMediaCodecsOptions();
            }
        }

        private void VideoTrimmer_SeekerValueChanging(object sender, EventArgs e)
        {
            // Pause if needed
            if (mediaControlPause.Visibility == Visibility.Visible)
            {
                PauseMedia();
            }

            // Change the position
            this.mediaStoryboard.Seek(new TimeSpan(Convert.ToInt64(this.videoTrimmer.SeekerPosition * TimeSpan.TicksPerMillisecond)));
        }

        private void AdvancedExpander_Expanded(object sender, RoutedEventArgs e)
        {
            if (advancedRow1.Height.Value == 0)
            {
                this.advancedRow1.Height = new GridLength(28, GridUnitType.Pixel);
                this.advancedRow2.Height = new GridLength(28, GridUnitType.Pixel);
                this.windowRoot.Height += 56;
            }
        }

        private void AdvancedExpander_Collapsed(object sender, RoutedEventArgs e)
        {
            if (advancedRow1.Height.Value == 28)
            {
                this.advancedRow1.Height = new GridLength(0, GridUnitType.Pixel);
                this.advancedRow2.Height = new GridLength(0, GridUnitType.Pixel);
                this.windowRoot.Height -= 56;
            }
        }

        private void PauseMedia()
        {
            // Pause media playback
            this.mediaStoryboard.Pause();
            this.mediaControlPause.Visibility = Visibility.Hidden;
            this.mediaControlPlay.Visibility = Visibility.Visible;
            this.videoTrimmer.IsSeekerLabelAlwaysVisible = false;
        }

        private void PlayMedia()
        {
            // Start playback
            this.mediaStoryboard.Resume();
            this.mediaControlPlay.Visibility = Visibility.Hidden;
            this.mediaControlPause.Visibility = Visibility.Visible;
            this.videoTrimmer.IsSeekerLabelAlwaysVisible = true;
        }

        private void OpenMedia(Uri source)
        {
            // Disable all controls until the file has been opened
            DisableAllControls();

            // Reset the failure counter 
            this.mediaFailedCount = 0;

            // Open the media
            this.mediaTimeline.Source = source;
            this.mediaStoryboard.Begin();
        }

        private void DisableAllControls()
        {
            // Stop playback
            this.mediaStoryboard.Stop();
            
            // Disable controls
            this.mediaElement.Visibility = Visibility.Hidden;
            this.mediaControlBack.Visibility = Visibility.Hidden;
            this.mediaControlPause.Visibility = Visibility.Hidden;
            this.mediaControlPlay.Visibility = Visibility.Hidden;
            this.videoTrimmer.IsEnabled = false;
            this.mediaFormat.IsEnabled = false;
            this.audioCodec.IsEnabled = false;
            this.audioQuality.IsEnabled = false;
            this.videoCodec.IsEnabled = false;
            this.videoQuality.IsEnabled = false;
            this.videoSize.IsEnabled = false;
            this.outputText.IsEnabled = false;
            this.browseOutput.IsEnabled = false;
            this.continueButton.IsEnabled = false;

            // Show the empty rectangle
            this.mediaElementBack.Visibility = Visibility.Visible;

            // Hide the playback controls
            this.mediaControlBack.Opacity = 0.0;
            this.mediaControlPause.Opacity = 0.0;
            this.mediaControlPlay.Opacity = 0.0;

            // Clear the source text box
            this.sourceText.Text = string.Empty;

            // Start the playback controls hide animation
            ((Storyboard)this.windowRoot.Resources["hidePlaybackControlsStoryboard"]).Begin();
        }

        private void UpdateVideoSizeOptions()
        {
            // Clear options
            this.videoSize.Items.Clear();

            // Add video sizes
            if (this.mediaElement.HasVideo)
            {
                if (((ComboBoxItem)this.mediaFormat.SelectedItem).Content.ToString().StartsWith("MP4", StringComparison.Ordinal) == true && this.mediaElement.NaturalVideoWidth > 640)
                {
                    double resizeFactor = 640.0 / this.mediaElement.NaturalVideoWidth;
                    uint newHeight = Convert.ToUInt32(this.mediaElement.NaturalVideoHeight * resizeFactor);

                    if (newHeight != 480)
                    {
                        this.videoSize.Items.Add(new PackedSize(640, newHeight));
                    }
                }
                else
                {
                    this.videoSize.Items.Add(new PackedSize(Convert.ToUInt32(this.mediaElement.NaturalVideoWidth), Convert.ToUInt32(this.mediaElement.NaturalVideoHeight)));

                    if (this.mediaElement.NaturalVideoHeight * this.mediaElement.NaturalVideoWidth > 1920 * 1080)
                    {
                        videoSize.Items.Add(new PackedSize(1920, 1080));
                    }

                    if (this.mediaElement.NaturalVideoHeight * this.mediaElement.NaturalVideoWidth > 1280 * 720)
                    {
                        videoSize.Items.Add(new PackedSize(1280, 720));
                    }
                }

                if (this.mediaElement.NaturalVideoHeight * this.mediaElement.NaturalVideoWidth > 640 * 480)
                {
                    this.videoSize.Items.Add(new PackedSize(640, 480));
                }

                if (this.mediaElement.NaturalVideoHeight * this.mediaElement.NaturalVideoWidth > 320 * 240)
                {
                    this.videoSize.Items.Add(new PackedSize(320, 240));
                }
            }
            else
            {
                this.videoSize.Items.Add(new PackedSize(640, 480));
            }

            this.videoSize.SelectedIndex = 0;
        }

        private void SetWindowsMediaCodecsOptions()
        {
            this.audioCodec.Items.Clear();
            this.audioCodec.Items.Add("WMA 9 Professional");
            this.audioCodec.Items.Add("WMA 9");
            this.audioCodec.SelectedIndex = 0;

            this.videoCodec.Items.Clear();
            this.videoCodec.Items.Add("VC-1 (SMPTE 421M)");
            this.videoCodec.Items.Add("WMV 9");
            this.videoCodec.Items.Add("WMV 8");
            this.videoCodec.SelectedIndex = 0;

            UpdateVideoSizeOptions();
        }

        private void SetMPEG4CodecsOptions()
        {
            this.videoCodec.Items.Clear();
            this.videoCodec.Items.Add("H.264 (AVC)");
            this.videoCodec.SelectedIndex = 0;

            this.audioCodec.Items.Clear();
            this.audioCodec.Items.Add("AAC");
            this.audioCodec.SelectedIndex = 0;

            UpdateVideoSizeOptions();
        }
    }
}
