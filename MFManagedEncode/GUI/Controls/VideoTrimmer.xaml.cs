// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

namespace MFManagedEncode.Gui
{
    using System;
    using System.Globalization;
    using System.Windows;
    using System.Windows.Controls;
    using System.Windows.Input;
    using System.Windows.Media;
    using System.Windows.Media.Animation;
    using MFManagedEncode.MediaFoundation;
    using MFManagedEncode.MediaFoundation.Com;
    using Microsoft.Win32;

    /// <summary>
    /// Interaction logic for VideoTrimmer.xaml
    /// </summary>
    public partial class VideoTrimmer : UserControl
    {
        private const double RULER_MARGIN = 24;
        private const double RULER_TOP = 35;
        private const double RULER_LINES_SPACE = 3;

        private DragObject dragging;

        private double seekerPositionAsPercentage;
        private double endPositionAsPercentage;
        private double startPositionAsPercentage;

        private double maxValue;
        private double minValue;

        private bool seekerLabelAlwaysVisible;

        public VideoTrimmer()
        {
            InitializeComponent();

            this.dragging = DragObject.None;

            this.seekerPositionAsPercentage = 0;
            this.startPositionAsPercentage = 0;
            this.endPositionAsPercentage = 1;

            this.minValue = 0;
            this.maxValue = 100;
        }

        public event EventHandler SeekerValueChanging;

        public event EventHandler SeekerValueChanged;

        // Controls that can be dragged
        private enum DragObject
        {
            None,
            Seeker,
            StartPosition,
            EndPosition
        }

        /// <summary>
        ///     Gets or sets the maximum value of the trimmer
        /// </summary>
        public double MaxValue
        {
            get
            {
                return this.maxValue;
            }

            set
            {
                this.maxValue = value;
            }
        }

        /// <summary>
        ///     Gets or sets the minimum value of the trimmer
        /// </summary>
        public double MinValue
        {
            get
            {
                return this.minValue;
            }

            set
            {
                this.minValue = value;
            }
        }

        /// <summary>
        ///     Gets or sets a value indicating whether the seeker time label is always visible
        /// </summary>
        public bool IsSeekerLabelAlwaysVisible
        {
            get
            {
                return this.seekerLabelAlwaysVisible;
            }

            set
            {
                this.seekerLabelAlwaysVisible = value;

                // Play the show/hide animation
                ((Storyboard)controlRoot.Resources[value ? "showSeekerLabel" : "hideSeekerLabel"]).Begin();
            }
        }

        /// <summary>
        ///     Gets or sets the seeker position
        /// </summary>
        public double SeekerPosition
        {
            get
            {
                return this.seekerPositionAsPercentage * (this.maxValue - this.minValue);
            }

            set
            {
                this.seekerPositionAsPercentage = (value > this.maxValue) ? 1 : ((value < this.minValue) ? 0 : value / (this.maxValue - this.minValue));
                UpdateSeekerPosition();
            }
        }

        /// <summary>
        ///     Gets or sets the start position
        /// </summary>
        public double StartPosition
        {
            get
            {
                return this.startPositionAsPercentage * (this.maxValue - this.minValue);
            }

            set
            {
                this.startPositionAsPercentage = (value > this.maxValue) ? 1 : ((value < this.minValue) ? 0 : value / (this.maxValue - this.minValue));
                Canvas.SetLeft(startPosition, GetPositionFromPercentage(startPosition));
            }
        }

        /// <summary>
        ///     Gets or sets the end position
        /// </summary>
        public double EndPosition
        {
            get
            {
                return this.endPositionAsPercentage * (this.maxValue - this.minValue);
            }

            set
            {
                this.endPositionAsPercentage = (value > this.maxValue) ? 1 : ((value < this.minValue) ? 0 : value / (this.maxValue - this.minValue));
                Canvas.SetLeft(endPosition, GetPositionFromPercentage(endPosition));
            }
        }

        protected override void OnRender(DrawingContext drawingContext)
        {
            Pen rulePen = new Pen(new SolidColorBrush(Color.FromRgb(100, 100, 100)), 1.0f);
            Pen ruleLimitsPen = new Pen(new SolidColorBrush(Colors.Black), 1.0f);

            if (IsEnabled == false)
            {
                rulePen.Brush.Opacity = 0.3;
                ruleLimitsPen.Brush.Opacity = 0.3;
            }

            // Draw the background
            drawingContext.DrawRectangle(new SolidColorBrush(Colors.White), null, new Rect(0, 0, this.ActualWidth, this.ActualHeight));

            // Draw each ruler line 
            for (double i = RULER_MARGIN; i < this.ActualWidth - RULER_MARGIN - 1; i += RULER_LINES_SPACE)
            {
                drawingContext.DrawLine(rulePen, new Point(i, RULER_TOP - 3), new Point(i, RULER_TOP + 3));
            }

            // Draw the ruler limits
            drawingContext.DrawLine(ruleLimitsPen, new Point(RULER_MARGIN, RULER_TOP - 5), new Point(RULER_MARGIN, RULER_TOP + 5));
            drawingContext.DrawLine(ruleLimitsPen, new Point(this.ActualWidth - RULER_MARGIN, RULER_TOP - 5), new Point(this.ActualWidth - RULER_MARGIN, RULER_TOP + 5));
        }

        private void VideoTrimmer_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            // Reposition all the controls
            UpdateSeekerPosition();
            Canvas.SetLeft(startPosition, GetPositionFromPercentage(startPosition));
            Canvas.SetLeft(endPosition, GetPositionFromPercentage(endPosition));
            Canvas.SetLeft(selection, Canvas.GetLeft(startPosition));
            selection.Width = Canvas.GetLeft(endPosition) - Canvas.GetLeft(selection);
        }

        private void VideoTrimmer_IsEnabledChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            // Play the enable/disable animations
            if ((bool)e.NewValue)
            {
                ((Storyboard)controlRoot.Resources["enableControlsStoryboard"]).Begin();
                this.InvalidateVisual();
            }
            else
            {
                ((Storyboard)controlRoot.Resources["disableControlsStoryboard"]).Begin();
                ((Storyboard)controlRoot.Resources["hideSeekerLabel"]).Begin();
                this.InvalidateVisual();
            }
        }

        private void VideoTrimmer_MouseMove(object sender, MouseEventArgs e)
        {
            // If not mouse left button is not pressed stop moving any control
            if (e.LeftButton == MouseButtonState.Released)
            {
                if (this.dragging == DragObject.Seeker)
                {
                    ((Storyboard)controlRoot.Resources[this.seekerLabelAlwaysVisible ? "showSeekerLabel" : "hideSeekerLabel"]).Begin();

                    // Fire the event
                    if (SeekerValueChanged != null)
                    {
                        SeekerValueChanged(this, null);
                    }
                }

                this.dragging = DragObject.None;
            }

            // Change the position of the dragged control
            if (this.dragging == DragObject.None)
            {
                return;
            }
            else if (this.dragging == DragObject.Seeker)
            {
                ((Storyboard)controlRoot.Resources["showSeekerLabel"]).Begin();

                SetObjectPosition(e.GetPosition(this).X, seeker);

                // Fire the event
                if (SeekerValueChanging != null)
                {
                    SeekerValueChanging(this, null);
                }
            }
            else if (this.dragging == DragObject.StartPosition)
            {
                SetObjectPosition(e.GetPosition(this).X, startPosition);
            }
            else if (this.dragging == DragObject.EndPosition)
            {
                SetObjectPosition(e.GetPosition(this).X, endPosition);
            }
        }

        private void VideoTrimmer_MouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            // Change the position of the dragged control
            if (this.dragging == DragObject.Seeker)
            {
                ((Storyboard)controlRoot.Resources[this.seekerLabelAlwaysVisible ? "showSeekerLabel" : "hideSeekerLabel"]).Begin();
                SetObjectPosition(e.GetPosition(this).X, seeker);

                // Fire the events
                if (SeekerValueChanging != null)
                {
                    SeekerValueChanging(this, null);
                }

                if (SeekerValueChanged != null)
                {
                    SeekerValueChanged(this, null);
                }
            }
            else if (this.dragging == DragObject.StartPosition)
            {
                SetObjectPosition(e.GetPosition(this).X, startPosition);
            }
            else if (this.dragging == DragObject.EndPosition)
            {
                SetObjectPosition(e.GetPosition(this).X, endPosition);
            }

            // Stop dragging any control
            this.dragging = DragObject.None;
        }

        private void VideoTrimmer_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            Point mousePosition = e.GetPosition(this);

            // Determinate which control will be moved
            if (mousePosition.Y > RULER_TOP - 5 && mousePosition.Y < RULER_TOP + 20)
            {
                this.dragging = DragObject.Seeker;
            }
            else if (mousePosition.Y <= RULER_TOP - 5 && mousePosition.Y > RULER_TOP - 30)
            {
                if (Math.Abs(Canvas.GetLeft(startPosition) + (startPosition.ActualWidth / 2) - mousePosition.X) < Math.Abs(Canvas.GetLeft(endPosition) + (endPosition.ActualWidth / 2) - mousePosition.X))
                {
                    this.dragging = DragObject.StartPosition;
                }
                else
                {
                    this.dragging = DragObject.EndPosition;
                }
            }
        }

        // Updates the position of the labels
        private void UpdateSeekerPosition()
        {
            Canvas.SetLeft(seeker, GetPositionFromPercentage(seeker));

            // Update the seeker label position and text
            TimeSpan seekerTime = TimeSpan.FromMilliseconds(this.seekerPositionAsPercentage * (this.maxValue - this.minValue));
            Canvas.SetLeft(seekerLabel, GetPositionFromPercentage(seeker) - (seekerLabel.ActualWidth / 2) + (seeker.ActualWidth / 2));
            seekerLabel.Content = 
                seekerTime.Hours.ToString("D2", NumberFormatInfo.InvariantInfo) + ":" +
                seekerTime.Minutes.ToString("D2", NumberFormatInfo.InvariantInfo) + ":" +
                seekerTime.Seconds.ToString("D2", NumberFormatInfo.InvariantInfo) + "." + 
                seekerTime.Milliseconds.ToString("D3", NumberFormatInfo.InvariantInfo);
        }

        private double GetPositionAsPercentage(UIElement element)
        {
            // Get the postion of a control as a percentage from its location
            if (element == seeker)
            {
                return (Canvas.GetLeft(seeker) + (seeker.ActualWidth / 2) - RULER_MARGIN) / (mainCanvas.ActualWidth - (RULER_MARGIN * 2));
            }

            if (element == startPosition)
            {
                return (Canvas.GetLeft(startPosition) + startPosition.ActualWidth - RULER_MARGIN) / (mainCanvas.ActualWidth - (RULER_MARGIN * 2));
            }

            if (element == endPosition)
            {
                return (Canvas.GetLeft(endPosition) - RULER_MARGIN) / (mainCanvas.ActualWidth - (RULER_MARGIN * 2));
            }

            return 0;
        }

        private double GetPositionFromPercentage(UIElement element)
        {
            // Get the position of a control as canvas coordinates from its percentage
            if (element == seeker)
            {
                return (this.seekerPositionAsPercentage * (mainCanvas.ActualWidth - (RULER_MARGIN * 2))) - (seeker.ActualWidth / 2) + RULER_MARGIN;
            }

            if (element == startPosition)
            {
                return (this.startPositionAsPercentage * (mainCanvas.ActualWidth - (RULER_MARGIN * 2))) - startPosition.ActualWidth + RULER_MARGIN;
            }

            if (element == endPosition)
            {
                return (this.endPositionAsPercentage * (mainCanvas.ActualWidth - (RULER_MARGIN * 2))) + RULER_MARGIN;
            }

            return 0;
        }

        private void SetObjectPosition(double newPosition, UIElement movingElement)
        {
            // Move the control
            if (movingElement == seeker)
            {
                if (newPosition < RULER_MARGIN)
                {
                    newPosition = RULER_MARGIN;
                }
                else if (newPosition > this.ActualWidth - RULER_MARGIN)
                {
                    newPosition = this.ActualWidth - RULER_MARGIN;
                }

                // Set the new position
                Canvas.SetLeft(seeker, newPosition - (seeker.ActualWidth / 2));

                // Save the position as percentage
                this.seekerPositionAsPercentage = GetPositionAsPercentage(seeker);

                // Update the label position
                UpdateSeekerPosition();
            }
            else if (movingElement == startPosition)
            {
                newPosition = newPosition + (startPosition.ActualWidth / 2);

                // Don't move the start position after the end position
                if (Canvas.GetLeft(endPosition) < newPosition)
                {
                    newPosition = Canvas.GetLeft(endPosition);
                }

                if (newPosition < RULER_MARGIN)
                {
                    newPosition = RULER_MARGIN;
                }
                else if (newPosition > this.ActualWidth - RULER_MARGIN)
                {
                    newPosition = this.ActualWidth - RULER_MARGIN;
                }

                // Move the start position
                Canvas.SetLeft(startPosition, newPosition - startPosition.ActualWidth);

                // Save the position as percentage
                this.startPositionAsPercentage = GetPositionAsPercentage(startPosition);

                // Resize and move the selection
                Canvas.SetLeft(selection, Canvas.GetLeft(startPosition));
                selection.Width = Canvas.GetLeft(endPosition) - Canvas.GetLeft(selection);
            }
            else if (movingElement == endPosition)
            {
                newPosition = newPosition - (endPosition.ActualWidth / 2);

                // Don't move the end position before the start position
                if (Canvas.GetLeft(startPosition) + startPosition.ActualWidth > newPosition)
                {
                    newPosition = Canvas.GetLeft(startPosition) + startPosition.ActualWidth;
                }

                if (newPosition < RULER_MARGIN)
                {
                    newPosition = RULER_MARGIN;
                }
                else if (newPosition > this.ActualWidth - RULER_MARGIN)
                {
                    newPosition = this.ActualWidth - RULER_MARGIN;
                }

                // Move the end position
                Canvas.SetLeft(endPosition, newPosition);

                // Save the position as percentage
                this.endPositionAsPercentage = GetPositionAsPercentage(endPosition);

                // Resize the selection
                selection.Width = Canvas.GetLeft(endPosition) - Canvas.GetLeft(selection);
            }
        }
    }
}
