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
    using System.Windows;
    using System.Windows.Documents;
    using System.Windows.Controls;
    using System.Windows.Input;
    using System.Windows.Media;
    using System.Windows.Media.Animation;
    using System.Windows.Threading;
    using MFManagedEncode.MediaFoundation;
    using MFManagedEncode.MediaFoundation.Com;

    /// <summary>
    /// Interaction logic for ConversionProgress.xaml
    /// </summary>
    public partial class ConversionProgress : Window
    {
        private ISimpleEncode encodeWorker;
        private DispatcherTimer progressTimer;
        private DateTime startTime;

        /// <summary>
        ///     Initializes a new instance of the ConversionProgress class
        /// </summary>
        /// <param name="arguments">Encode arguments</param>
        /// <param name="useSinkWriter">False to use the Transcode API, true to use Sink Writer</param>
        public ConversionProgress(Dictionary<string, object> arguments, bool useSinkWriter)
        {
            // Start Media Foundation 
            MFHelper.MFStartup();

            this.InitializeComponent();

            this.startTime = DateTime.Now;
            this.progressTimer = new DispatcherTimer();
            this.progressTimer.Interval = TimeSpan.FromMilliseconds(500);
            this.progressTimer.Tick += ProgressTimer_Tick;

            if (useSinkWriter)
            {
                this.encodeWorker = new SimpleSinkWriterEncode();
            }
            else
            {
                this.encodeWorker = new SimpleFastEncode();
            }

            // Start transcoding 
            this.StartEncode(arguments);
        }

        public ConversionProgress()
        {
            // Shutdown Media Foundation 
            MFHelper.MFShutdown();

            InitializeComponent();
        }

        private void ProgressTimer_Tick(object sender, EventArgs e)
        {
            // Calculate the remaining time
            double currentProgress = this.conversionProgress.Value;

            if (currentProgress == 0)
            {
                return;
            }

            TimeSpan timeRemaining = TimeSpan.FromTicks(Convert.ToInt64((DateTime.Now.Ticks - this.startTime.Ticks) * ((100 - currentProgress) / currentProgress)));

            if (Math.Truncate(timeRemaining.TotalDays) == 1)
            {
                timeLabel.Text = "About " + Math.Truncate(timeRemaining.TotalDays) + " day remaining";
            }
            else if (Math.Truncate(timeRemaining.TotalDays) > 0)
            {
                timeLabel.Text = "About " + Math.Truncate(timeRemaining.TotalDays) + " days remaining";
            }
            else if (Math.Truncate(timeRemaining.TotalHours) == 1)
            {
                timeLabel.Text = "About " + Math.Truncate(timeRemaining.TotalHours) + " hour remaining";
            }
            else if (Math.Truncate(timeRemaining.TotalHours) > 0)
            {
                timeLabel.Text = "About " + Math.Truncate(timeRemaining.TotalHours) + " hours remaining";
            }
            else if (Math.Truncate(timeRemaining.TotalMinutes) == 1)
            {
                timeLabel.Text = "About " + Math.Truncate(timeRemaining.TotalMinutes) + " minute remaining";
            }
            else if (Math.Truncate(timeRemaining.TotalMinutes) > 0)
            {
                timeLabel.Text = "About " + Math.Truncate(timeRemaining.TotalMinutes) + " minutes remaining";
            }
            else if (Math.Truncate(timeRemaining.TotalSeconds) == 1)
            {
                timeLabel.Text = "About " + Math.Truncate(timeRemaining.TotalSeconds) + " second remaining";
            }
            else if (Math.Truncate(timeRemaining.TotalSeconds) > 0)
            {
                timeLabel.Text = "About " + Math.Truncate(timeRemaining.TotalSeconds) + " seconds remaining";
            }
            else
            {
                timeLabel.Text = "Less than one second remaining";
            }
        }

        private void StartEncode(Dictionary<string, object> encodeArgs)
        {
            string destinationFileName = string.Empty;
            string outputPath = string.Empty;
            string newFormat = (string)encodeArgs["OutputExtension"];

            // Get the FileName and path
            string fileName = System.IO.Path.GetFileName((string)encodeArgs["InputURL"]);

            if (encodeArgs.ContainsKey("OutputURL") == false)
            {
                outputPath = System.IO.Path.GetDirectoryName((string)encodeArgs["InputURL"]);
            }
            else
            {
                outputPath = (string)encodeArgs["OutputURL"];
            }
            
            // Generate the output file name
            if (System.IO.File.Exists(outputPath + "\\" + fileName.Split('.')[0] + "." + newFormat))
            {
                // If a file with the default name already exists try again adding a number
                for (int i = 2; true; i++)
                {
                    if (System.IO.File.Exists(outputPath + "\\" + fileName.Split('.')[0] + " (" + i + ")." + newFormat) == false)
                    {
                        destinationFileName = fileName.Split('.')[0] + " (" + i + ")." + newFormat;
                        break;
                    }
                }
            }
            else
            {
                // Use the default file name if a file with the same name doesn't exist
                destinationFileName = fileName.Split('.')[0] + "." + newFormat;
            }

            encodeArgs.Remove("OutputURL");
            encodeArgs.Add("OutputURL", outputPath + "\\" + destinationFileName);

            if (encodeArgs.ContainsKey("AudioFormat") == false)
            {
                encodeArgs.Add("AudioFormat", new AudioFormat(Consts.MFAudioFormat_WMAudioV9));
            }

            if (encodeArgs.ContainsKey("VideoFormat") == false)
            {
                encodeArgs.Add("VideoFormat", new VideoFormat(Consts.MFVideoFormat_WVC1));

                ((VideoFormat)encodeArgs["VideoFormat"]).FrameSize.Packed = 0;
                ((VideoFormat)encodeArgs["VideoFormat"]).FrameRate.Packed = 0;
            }

            if (encodeArgs.ContainsKey("StartTime") == false)
            {
                encodeArgs.Add("StartTime", (ulong)0);
            }

            if (encodeArgs.ContainsKey("EndTime") == false)
            {
                encodeArgs.Add("EndTime", (ulong)0);
            }

            // Update controls text
            this.pathLabel.Inlines.Clear();
            this.pathLabel.Inlines.Add(new Run("from "));
            this.pathLabel.Inlines.Add(new Bold(new Run(System.IO.Path.GetFileName((string)encodeArgs["InputURL"]))));
            this.pathLabel.Inlines.Add(new Run(" to "));
            this.pathLabel.Inlines.Add(new Bold(new Run(System.IO.Path.GetFileName((string)encodeArgs["OutputURL"]))));

            this.timeLabel.Inlines.Clear();

            // Bind the encode events and start the operation
            this.encodeWorker.EncodeProgress += EncodeProgress;
            this.encodeWorker.EncodeCompleted += EncodeCompleted;
            this.encodeWorker.EncodeError += EncodeError;

            this.startTime = DateTime.Now;

            this.progressTimer.Start();

            this.encodeWorker.Encode((string)encodeArgs["InputURL"], (string)encodeArgs["OutputURL"], (AudioFormat)encodeArgs["AudioFormat"], (VideoFormat)encodeArgs["VideoFormat"], (ulong)encodeArgs["StartTime"], (ulong)encodeArgs["EndTime"]);
        }

        private void EncodeError(Exception e)
        {
            if (this.Dispatcher.CheckAccess())
            {
                MessageBox.Show(e.Message, "Error", MessageBoxButton.OK, MessageBoxImage.Error, MessageBoxResult.OK, MessageBoxOptions.None);
                Close();
            }
            else
            {
                this.Dispatcher.Invoke(DispatcherPriority.Normal, new EncodeErrorHandler(this.EncodeError), e);
            }
        }

        private void EncodeCompleted(object sender, EventArgs e)
        {
            if (this.Dispatcher.CheckAccess())
            {
                this.progressTimer.Stop();
                this.Close();
            }
            else
            {
                this.Dispatcher.Invoke(DispatcherPriority.Normal, new EventHandler(this.EncodeCompleted), sender, e);
            }
        }

        private void EncodeProgress(double progress)
        {
            if (this.conversionProgress.Dispatcher.CheckAccess())
            {
                if (progress < this.conversionProgress.Value)
                {
                    return;
                }

                // Animate the progress bar
                Duration duration = new Duration(TimeSpan.FromSeconds(1));
                DoubleAnimation newValue = new DoubleAnimation((progress <= 100) ? progress : 100, duration);

                this.conversionProgress.BeginAnimation(ProgressBar.ValueProperty, newValue);
            }
            else
            {
                this.Dispatcher.Invoke(DispatcherPriority.Normal, new EncodeProgressHandler(EncodeProgress), progress);
            }
        }

        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            // If encoding is still going then cancel the operation, this will fire the EncodeCompleted event
            if (this.encodeWorker.IsBusy())
            {
                this.encodeWorker.CancelAsync();
                e.Cancel = true;
            }
        }

        private void CancelButton_Click(object sender, RoutedEventArgs e)
        {
            this.Close();
        }
    }
}
