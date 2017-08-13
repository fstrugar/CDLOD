namespace Wizard
{
   partial class WizardForm
   {
      /// <summary>
      /// Required designer variable.
      /// </summary>
      private System.ComponentModel.IContainer components = null;

      /// <summary>
      /// Clean up any resources being used.
      /// </summary>
      /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
      protected override void Dispose( bool disposing )
      {
         if( disposing && ( components != null ) )
         {
            components.Dispose( );
         }
         base.Dispose( disposing );
      }

      #region Windows Form Designer generated code

      /// <summary>
      /// Required method for Designer support - do not modify
      /// the contents of this method with the code editor.
      /// </summary>
      private void InitializeComponent( )
      {
         this.menuStrip1 = new System.Windows.Forms.MenuStrip( );
         this.fileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem( );
         this.openToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem( );
         this.saveAsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem( );
         this.toolStripMenuItem1 = new System.Windows.Forms.ToolStripSeparator( );
         this.convertTBMPToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem( );
         this.labelHeightmapFile = new System.Windows.Forms.Label( );
         this.textBoxHeightmap = new System.Windows.Forms.TextBox( );
         this.buttonBrowseHeightmap = new System.Windows.Forms.Button( );
         this.labelWorldX = new System.Windows.Forms.Label( );
         this.numericUpDownWorldSizeX = new System.Windows.Forms.NumericUpDown( );
         this.numericUpDownWorldSizeY = new System.Windows.Forms.NumericUpDown( );
         this.labelWorldY = new System.Windows.Forms.Label( );
         this.labelWorldZ = new System.Windows.Forms.Label( );
         this.labelStats = new System.Windows.Forms.Label( );
         this.richTextBoxStats = new System.Windows.Forms.RichTextBox( );
         this.labelOverlayImage = new System.Windows.Forms.Label( );
         this.textBoxOverlayImage = new System.Windows.Forms.TextBox( );
         this.buttonBrowseOverlay = new System.Windows.Forms.Button( );
         this.label7 = new System.Windows.Forms.Label( );
         this.textBoxOutputSDLOD = new System.Windows.Forms.TextBox( );
         this.buttonBrowseOutputSDLOD = new System.Windows.Forms.Button( );
         this.buttonGo = new System.Windows.Forms.Button( );
         this.checkBoxOverlayImgCompression = new System.Windows.Forms.CheckBox( );
         this.checkBoxHeightmapDXT5Compression = new System.Windows.Forms.CheckBox( );
         this.checkBoxUseZLIBCompression = new System.Windows.Forms.CheckBox( );
         this.groupBoxSourceData = new System.Windows.Forms.GroupBox( );
         this.buttonClearOverlaymapPath = new System.Windows.Forms.Button( );
         this.labelNormalMapExplanation = new System.Windows.Forms.Label( );
         this.labelNormalMap = new System.Windows.Forms.Label( );
         this.groupBoxWorldDimensions = new System.Windows.Forms.GroupBox( );
         this.numericUpDownWorldSizeZ = new System.Windows.Forms.NumericUpDown( );
         this.groupBoxDataCompression = new System.Windows.Forms.GroupBox( );
         this.groupBoxStreanableDataSettings = new System.Windows.Forms.GroupBox( );
         this.numericUpDownOverlaymapLODoffset = new System.Windows.Forms.NumericUpDown( );
         this.numericUpDownNormalmapLODoffset = new System.Windows.Forms.NumericUpDown( );
         this.numericUpDownHeightmapLODoffset = new FixedNumericUpDown( );
         this.comboBoxOverlaymapBlockSize = new System.Windows.Forms.ComboBox( );
         this.comboBoxNormalmapBlockSize = new System.Windows.Forms.ComboBox( );
         this.comboBoxHeightmapBlockSize = new System.Windows.Forms.ComboBox( );
         this.label18 = new System.Windows.Forms.Label( );
         this.label15 = new System.Windows.Forms.Label( );
         this.label11 = new System.Windows.Forms.Label( );
         this.label17 = new System.Windows.Forms.Label( );
         this.label14 = new System.Windows.Forms.Label( );
         this.label10 = new System.Windows.Forms.Label( );
         this.labelOverlayMap = new System.Windows.Forms.Label( );
         this.labelNormalMapBlockSize = new System.Windows.Forms.Label( );
         this.labelHeightmap = new System.Windows.Forms.Label( );
         this.groupBoxLODSettings = new System.Windows.Forms.GroupBox( );
         this.labelRenderGridResolutionMultiplier = new System.Windows.Forms.Label( );
         this.labelLeafQuadtreeNodeSize = new System.Windows.Forms.Label( );
         this.labelLODLevelCount = new System.Windows.Forms.Label( );
         this.numericUpDownLODLevelCount = new System.Windows.Forms.NumericUpDown( );
         this.comboBoxRenderGridResolutionMultiplier = new System.Windows.Forms.ComboBox( );
         this.comboBoxLeafQuadtreeNodeSize = new System.Windows.Forms.ComboBox( );
         this.groupBoxRendaring = new System.Windows.Forms.GroupBox( );
         this.labelLODLevelDistanceRatio = new System.Windows.Forms.Label( );
         this.labelMaxViewRange = new System.Windows.Forms.Label( );
         this.labelMinViewRange = new System.Windows.Forms.Label( );
         this.numericUpDownLODLevelDistanceRatio = new System.Windows.Forms.NumericUpDown( );
         this.numericUpDownMaxViewRange = new System.Windows.Forms.NumericUpDown( );
         this.numericUpDownMinViewRange = new System.Windows.Forms.NumericUpDown( );
         this.groupBoxDetailMap = new System.Windows.Forms.GroupBox( );
         this.checkBoxDetailMapEnabled = new System.Windows.Forms.CheckBox( );
         this.numericUpDownDetailMapSizeZ = new System.Windows.Forms.NumericUpDown( );
         this.numericUpDownDetailMapLODLevelsAffected = new System.Windows.Forms.NumericUpDown( );
         this.numericUpDownDetailMapSizeX = new System.Windows.Forms.NumericUpDown( );
         this.numericUpDownDetailMapSizeY = new System.Windows.Forms.NumericUpDown( );
         this.label29 = new System.Windows.Forms.Label( );
         this.label28 = new System.Windows.Forms.Label( );
         this.label25 = new System.Windows.Forms.Label( );
         this.label26 = new System.Windows.Forms.Label( );
         this.menuStrip1.SuspendLayout( );
         ( (System.ComponentModel.ISupportInitialize)( this.numericUpDownWorldSizeX ) ).BeginInit( );
         ( (System.ComponentModel.ISupportInitialize)( this.numericUpDownWorldSizeY ) ).BeginInit( );
         this.groupBoxSourceData.SuspendLayout( );
         this.groupBoxWorldDimensions.SuspendLayout( );
         ( (System.ComponentModel.ISupportInitialize)( this.numericUpDownWorldSizeZ ) ).BeginInit( );
         this.groupBoxDataCompression.SuspendLayout( );
         this.groupBoxStreanableDataSettings.SuspendLayout( );
         ( (System.ComponentModel.ISupportInitialize)( this.numericUpDownOverlaymapLODoffset ) ).BeginInit( );
         ( (System.ComponentModel.ISupportInitialize)( this.numericUpDownNormalmapLODoffset ) ).BeginInit( );
         ( (System.ComponentModel.ISupportInitialize)( this.numericUpDownHeightmapLODoffset ) ).BeginInit( );
         this.groupBoxLODSettings.SuspendLayout( );
         ( (System.ComponentModel.ISupportInitialize)( this.numericUpDownLODLevelCount ) ).BeginInit( );
         this.groupBoxRendaring.SuspendLayout( );
         ( (System.ComponentModel.ISupportInitialize)( this.numericUpDownLODLevelDistanceRatio ) ).BeginInit( );
         ( (System.ComponentModel.ISupportInitialize)( this.numericUpDownMaxViewRange ) ).BeginInit( );
         ( (System.ComponentModel.ISupportInitialize)( this.numericUpDownMinViewRange ) ).BeginInit( );
         this.groupBoxDetailMap.SuspendLayout( );
         ( (System.ComponentModel.ISupportInitialize)( this.numericUpDownDetailMapSizeZ ) ).BeginInit( );
         ( (System.ComponentModel.ISupportInitialize)( this.numericUpDownDetailMapLODLevelsAffected ) ).BeginInit( );
         ( (System.ComponentModel.ISupportInitialize)( this.numericUpDownDetailMapSizeX ) ).BeginInit( );
         ( (System.ComponentModel.ISupportInitialize)( this.numericUpDownDetailMapSizeY ) ).BeginInit( );
         this.SuspendLayout( );
         // 
         // menuStrip1
         // 
         this.menuStrip1.Items.AddRange( new System.Windows.Forms.ToolStripItem[] {
            this.fileToolStripMenuItem} );
         this.menuStrip1.Location = new System.Drawing.Point( 0, 0 );
         this.menuStrip1.Name = "menuStrip1";
         this.menuStrip1.Size = new System.Drawing.Size( 594, 24 );
         this.menuStrip1.TabIndex = 2;
         this.menuStrip1.Text = "menuStrip1";
         // 
         // fileToolStripMenuItem
         // 
         this.fileToolStripMenuItem.DropDownItems.AddRange( new System.Windows.Forms.ToolStripItem[] {
            this.openToolStripMenuItem,
            this.saveAsToolStripMenuItem,
            this.toolStripMenuItem1,
            this.convertTBMPToolStripMenuItem} );
         this.fileToolStripMenuItem.Name = "fileToolStripMenuItem";
         this.fileToolStripMenuItem.Size = new System.Drawing.Size( 37, 20 );
         this.fileToolStripMenuItem.Text = "File";
         // 
         // openToolStripMenuItem
         // 
         this.openToolStripMenuItem.Name = "openToolStripMenuItem";
         this.openToolStripMenuItem.Size = new System.Drawing.Size( 165, 22 );
         this.openToolStripMenuItem.Text = "Open";
         this.openToolStripMenuItem.Click += new System.EventHandler( this.openToolStripMenuItem_Click );
         // 
         // saveAsToolStripMenuItem
         // 
         this.saveAsToolStripMenuItem.Name = "saveAsToolStripMenuItem";
         this.saveAsToolStripMenuItem.Size = new System.Drawing.Size( 165, 22 );
         this.saveAsToolStripMenuItem.Text = "Save As";
         this.saveAsToolStripMenuItem.Click += new System.EventHandler( this.saveAsToolStripMenuItem_Click );
         // 
         // toolStripMenuItem1
         // 
         this.toolStripMenuItem1.Name = "toolStripMenuItem1";
         this.toolStripMenuItem1.Size = new System.Drawing.Size( 162, 6 );
         // 
         // convertTBMPToolStripMenuItem
         // 
         this.convertTBMPToolStripMenuItem.Name = "convertTBMPToolStripMenuItem";
         this.convertTBMPToolStripMenuItem.Size = new System.Drawing.Size( 165, 22 );
         this.convertTBMPToolStripMenuItem.Text = "Convert to TBMP";
         // 
         // labelHeightmapFile
         // 
         this.labelHeightmapFile.AutoSize = true;
         this.labelHeightmapFile.Location = new System.Drawing.Point( 7, 26 );
         this.labelHeightmapFile.Name = "labelHeightmapFile";
         this.labelHeightmapFile.Size = new System.Drawing.Size( 75, 13 );
         this.labelHeightmapFile.TabIndex = 3;
         this.labelHeightmapFile.Text = "heightmap file:";
         // 
         // textBoxHeightmap
         // 
         this.textBoxHeightmap.Location = new System.Drawing.Point( 88, 24 );
         this.textBoxHeightmap.Name = "textBoxHeightmap";
         this.textBoxHeightmap.ReadOnly = true;
         this.textBoxHeightmap.Size = new System.Drawing.Size( 394, 20 );
         this.textBoxHeightmap.TabIndex = 4;
         // 
         // buttonBrowseHeightmap
         // 
         this.buttonBrowseHeightmap.Location = new System.Drawing.Point( 488, 24 );
         this.buttonBrowseHeightmap.Name = "buttonBrowseHeightmap";
         this.buttonBrowseHeightmap.Size = new System.Drawing.Size( 76, 20 );
         this.buttonBrowseHeightmap.TabIndex = 5;
         this.buttonBrowseHeightmap.Text = "Browse";
         this.buttonBrowseHeightmap.UseVisualStyleBackColor = true;
         this.buttonBrowseHeightmap.Click += new System.EventHandler( this.buttonBrowseHeightmap_Click );
         // 
         // labelWorldX
         // 
         this.labelWorldX.AutoSize = true;
         this.labelWorldX.Location = new System.Drawing.Point( 9, 29 );
         this.labelWorldX.Name = "labelWorldX";
         this.labelWorldX.Size = new System.Drawing.Size( 40, 13 );
         this.labelWorldX.TabIndex = 6;
         this.labelWorldX.Text = "Size X:";
         // 
         // numericUpDownWorldSizeX
         // 
         this.numericUpDownWorldSizeX.DecimalPlaces = 1;
         this.numericUpDownWorldSizeX.Increment = new decimal( new int[] {
            1000,
            0,
            0,
            0} );
         this.numericUpDownWorldSizeX.Location = new System.Drawing.Point( 88, 25 );
         this.numericUpDownWorldSizeX.Maximum = new decimal( new int[] {
            1000000,
            0,
            0,
            0} );
         this.numericUpDownWorldSizeX.Minimum = new decimal( new int[] {
            1,
            0,
            0,
            0} );
         this.numericUpDownWorldSizeX.Name = "numericUpDownWorldSizeX";
         this.numericUpDownWorldSizeX.Size = new System.Drawing.Size( 122, 20 );
         this.numericUpDownWorldSizeX.TabIndex = 7;
         this.numericUpDownWorldSizeX.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
         this.numericUpDownWorldSizeX.ThousandsSeparator = true;
         this.numericUpDownWorldSizeX.Value = new decimal( new int[] {
            10000,
            0,
            0,
            0} );
         // 
         // numericUpDownWorldSizeY
         // 
         this.numericUpDownWorldSizeY.DecimalPlaces = 1;
         this.numericUpDownWorldSizeY.Increment = new decimal( new int[] {
            1000,
            0,
            0,
            0} );
         this.numericUpDownWorldSizeY.Location = new System.Drawing.Point( 88, 48 );
         this.numericUpDownWorldSizeY.Maximum = new decimal( new int[] {
            1000000,
            0,
            0,
            0} );
         this.numericUpDownWorldSizeY.Minimum = new decimal( new int[] {
            1,
            0,
            0,
            0} );
         this.numericUpDownWorldSizeY.Name = "numericUpDownWorldSizeY";
         this.numericUpDownWorldSizeY.Size = new System.Drawing.Size( 122, 20 );
         this.numericUpDownWorldSizeY.TabIndex = 9;
         this.numericUpDownWorldSizeY.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
         this.numericUpDownWorldSizeY.ThousandsSeparator = true;
         this.numericUpDownWorldSizeY.Value = new decimal( new int[] {
            1000,
            0,
            0,
            0} );
         // 
         // labelWorldY
         // 
         this.labelWorldY.AutoSize = true;
         this.labelWorldY.Location = new System.Drawing.Point( 9, 52 );
         this.labelWorldY.Name = "labelWorldY";
         this.labelWorldY.Size = new System.Drawing.Size( 40, 13 );
         this.labelWorldY.TabIndex = 8;
         this.labelWorldY.Text = "Size Y:";
         // 
         // labelWorldZ
         // 
         this.labelWorldZ.AutoSize = true;
         this.labelWorldZ.Location = new System.Drawing.Point( 9, 75 );
         this.labelWorldZ.Name = "labelWorldZ";
         this.labelWorldZ.Size = new System.Drawing.Size( 78, 13 );
         this.labelWorldZ.TabIndex = 10;
         this.labelWorldZ.Text = "Size Z (height):";
         // 
         // labelStats
         // 
         this.labelStats.AutoSize = true;
         this.labelStats.Location = new System.Drawing.Point( 9, 462 );
         this.labelStats.Name = "labelStats";
         this.labelStats.Size = new System.Drawing.Size( 34, 13 );
         this.labelStats.TabIndex = 12;
         this.labelStats.Text = "Stats:";
         // 
         // richTextBoxStats
         // 
         this.richTextBoxStats.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
         this.richTextBoxStats.Cursor = System.Windows.Forms.Cursors.Default;
         this.richTextBoxStats.Location = new System.Drawing.Point( 12, 478 );
         this.richTextBoxStats.Name = "richTextBoxStats";
         this.richTextBoxStats.ReadOnly = true;
         this.richTextBoxStats.Size = new System.Drawing.Size( 570, 73 );
         this.richTextBoxStats.TabIndex = 13;
         this.richTextBoxStats.Text = "No heightmap selected";
         // 
         // labelOverlayImage
         // 
         this.labelOverlayImage.AutoSize = true;
         this.labelOverlayImage.Location = new System.Drawing.Point( 7, 69 );
         this.labelOverlayImage.Name = "labelOverlayImage";
         this.labelOverlayImage.Size = new System.Drawing.Size( 75, 13 );
         this.labelOverlayImage.TabIndex = 3;
         this.labelOverlayImage.Text = "overlay image:";
         // 
         // textBoxOverlayImage
         // 
         this.textBoxOverlayImage.Location = new System.Drawing.Point( 88, 66 );
         this.textBoxOverlayImage.Name = "textBoxOverlayImage";
         this.textBoxOverlayImage.ReadOnly = true;
         this.textBoxOverlayImage.Size = new System.Drawing.Size( 377, 20 );
         this.textBoxOverlayImage.TabIndex = 4;
         // 
         // buttonBrowseOverlay
         // 
         this.buttonBrowseOverlay.Location = new System.Drawing.Point( 488, 66 );
         this.buttonBrowseOverlay.Name = "buttonBrowseOverlay";
         this.buttonBrowseOverlay.Size = new System.Drawing.Size( 76, 20 );
         this.buttonBrowseOverlay.TabIndex = 5;
         this.buttonBrowseOverlay.Text = "Browse";
         this.buttonBrowseOverlay.UseVisualStyleBackColor = true;
         this.buttonBrowseOverlay.Click += new System.EventHandler( this.buttonBrowseOverlay_Click );
         // 
         // label7
         // 
         this.label7.AutoSize = true;
         this.label7.Location = new System.Drawing.Point( 9, 566 );
         this.label7.Name = "label7";
         this.label7.Size = new System.Drawing.Size( 89, 13 );
         this.label7.TabIndex = 3;
         this.label7.Text = "Output .sdlod file:";
         // 
         // textBoxOutputSDLOD
         // 
         this.textBoxOutputSDLOD.Location = new System.Drawing.Point( 12, 582 );
         this.textBoxOutputSDLOD.Name = "textBoxOutputSDLOD";
         this.textBoxOutputSDLOD.Size = new System.Drawing.Size( 489, 20 );
         this.textBoxOutputSDLOD.TabIndex = 4;
         // 
         // buttonBrowseOutputSDLOD
         // 
         this.buttonBrowseOutputSDLOD.Location = new System.Drawing.Point( 507, 582 );
         this.buttonBrowseOutputSDLOD.Name = "buttonBrowseOutputSDLOD";
         this.buttonBrowseOutputSDLOD.Size = new System.Drawing.Size( 75, 20 );
         this.buttonBrowseOutputSDLOD.TabIndex = 5;
         this.buttonBrowseOutputSDLOD.Text = "Browse";
         this.buttonBrowseOutputSDLOD.UseVisualStyleBackColor = true;
         this.buttonBrowseOutputSDLOD.Click += new System.EventHandler( this.buttonBrowseOutputSDLOD_Click );
         // 
         // buttonGo
         // 
         this.buttonGo.Font = new System.Drawing.Font( "Verdana", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ( (byte)( 0 ) ) );
         this.buttonGo.Location = new System.Drawing.Point( 12, 608 );
         this.buttonGo.Name = "buttonGo";
         this.buttonGo.Size = new System.Drawing.Size( 570, 23 );
         this.buttonGo.TabIndex = 14;
         this.buttonGo.Text = "Go";
         this.buttonGo.UseVisualStyleBackColor = true;
         this.buttonGo.Click += new System.EventHandler( this.buttonGo_Click );
         // 
         // checkBoxOverlayImgCompression
         // 
         this.checkBoxOverlayImgCompression.AutoSize = true;
         this.checkBoxOverlayImgCompression.Checked = true;
         this.checkBoxOverlayImgCompression.CheckState = System.Windows.Forms.CheckState.Checked;
         this.checkBoxOverlayImgCompression.Location = new System.Drawing.Point( 12, 46 );
         this.checkBoxOverlayImgCompression.Name = "checkBoxOverlayImgCompression";
         this.checkBoxOverlayImgCompression.Size = new System.Drawing.Size( 211, 17 );
         this.checkBoxOverlayImgCompression.TabIndex = 15;
         this.checkBoxOverlayImgCompression.Text = "Overlay image uses DXT1 compression";
         this.checkBoxOverlayImgCompression.UseVisualStyleBackColor = true;
         // 
         // checkBoxHeightmapDXT5Compression
         // 
         this.checkBoxHeightmapDXT5Compression.AutoSize = true;
         this.checkBoxHeightmapDXT5Compression.Checked = true;
         this.checkBoxHeightmapDXT5Compression.CheckState = System.Windows.Forms.CheckState.Checked;
         this.checkBoxHeightmapDXT5Compression.Location = new System.Drawing.Point( 12, 21 );
         this.checkBoxHeightmapDXT5Compression.Name = "checkBoxHeightmapDXT5Compression";
         this.checkBoxHeightmapDXT5Compression.Size = new System.Drawing.Size( 218, 17 );
         this.checkBoxHeightmapDXT5Compression.TabIndex = 16;
         this.checkBoxHeightmapDXT5Compression.Text = "Heightmap uses DXT5_NM compression";
         this.checkBoxHeightmapDXT5Compression.UseVisualStyleBackColor = true;
         // 
         // checkBoxUseZLIBCompression
         // 
         this.checkBoxUseZLIBCompression.AutoSize = true;
         this.checkBoxUseZLIBCompression.Checked = true;
         this.checkBoxUseZLIBCompression.CheckState = System.Windows.Forms.CheckState.Checked;
         this.checkBoxUseZLIBCompression.Location = new System.Drawing.Point( 12, 71 );
         this.checkBoxUseZLIBCompression.Name = "checkBoxUseZLIBCompression";
         this.checkBoxUseZLIBCompression.Size = new System.Drawing.Size( 218, 17 );
         this.checkBoxUseZLIBCompression.TabIndex = 15;
         this.checkBoxUseZLIBCompression.Text = "Use zlib compression for streamable data";
         this.checkBoxUseZLIBCompression.UseVisualStyleBackColor = true;
         // 
         // groupBoxSourceData
         // 
         this.groupBoxSourceData.Controls.Add( this.buttonClearOverlaymapPath );
         this.groupBoxSourceData.Controls.Add( this.labelNormalMapExplanation );
         this.groupBoxSourceData.Controls.Add( this.labelNormalMap );
         this.groupBoxSourceData.Controls.Add( this.labelHeightmapFile );
         this.groupBoxSourceData.Controls.Add( this.labelOverlayImage );
         this.groupBoxSourceData.Controls.Add( this.textBoxHeightmap );
         this.groupBoxSourceData.Controls.Add( this.textBoxOverlayImage );
         this.groupBoxSourceData.Controls.Add( this.buttonBrowseHeightmap );
         this.groupBoxSourceData.Controls.Add( this.buttonBrowseOverlay );
         this.groupBoxSourceData.Location = new System.Drawing.Point( 12, 42 );
         this.groupBoxSourceData.Name = "groupBoxSourceData";
         this.groupBoxSourceData.Size = new System.Drawing.Size( 570, 96 );
         this.groupBoxSourceData.TabIndex = 17;
         this.groupBoxSourceData.TabStop = false;
         this.groupBoxSourceData.Text = "Source data";
         // 
         // buttonClearOverlaymapPath
         // 
         this.buttonClearOverlaymapPath.Font = new System.Drawing.Font( "Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ( (byte)( 0 ) ) );
         this.buttonClearOverlaymapPath.Location = new System.Drawing.Point( 467, 66 );
         this.buttonClearOverlaymapPath.Name = "buttonClearOverlaymapPath";
         this.buttonClearOverlaymapPath.Size = new System.Drawing.Size( 20, 20 );
         this.buttonClearOverlaymapPath.TabIndex = 6;
         this.buttonClearOverlaymapPath.Text = "X";
         this.buttonClearOverlaymapPath.UseVisualStyleBackColor = true;
         this.buttonClearOverlaymapPath.Click += new System.EventHandler( this.buttonClearOverlaymapPath_Click );
         // 
         // labelNormalMapExplanation
         // 
         this.labelNormalMapExplanation.AutoSize = true;
         this.labelNormalMapExplanation.Location = new System.Drawing.Point( 85, 48 );
         this.labelNormalMapExplanation.Name = "labelNormalMapExplanation";
         this.labelNormalMapExplanation.Size = new System.Drawing.Size( 125, 13 );
         this.labelNormalMapExplanation.TabIndex = 3;
         this.labelNormalMapExplanation.Text = "(generated automatically)";
         // 
         // labelNormalMap
         // 
         this.labelNormalMap.AutoSize = true;
         this.labelNormalMap.Location = new System.Drawing.Point( 8, 48 );
         this.labelNormalMap.Name = "labelNormalMap";
         this.labelNormalMap.Size = new System.Drawing.Size( 64, 13 );
         this.labelNormalMap.TabIndex = 3;
         this.labelNormalMap.Text = "normal map:";
         // 
         // groupBoxWorldDimensions
         // 
         this.groupBoxWorldDimensions.Controls.Add( this.labelWorldX );
         this.groupBoxWorldDimensions.Controls.Add( this.numericUpDownWorldSizeX );
         this.groupBoxWorldDimensions.Controls.Add( this.labelWorldY );
         this.groupBoxWorldDimensions.Controls.Add( this.numericUpDownWorldSizeZ );
         this.groupBoxWorldDimensions.Controls.Add( this.numericUpDownWorldSizeY );
         this.groupBoxWorldDimensions.Controls.Add( this.labelWorldZ );
         this.groupBoxWorldDimensions.Location = new System.Drawing.Point( 12, 144 );
         this.groupBoxWorldDimensions.Name = "groupBoxWorldDimensions";
         this.groupBoxWorldDimensions.Size = new System.Drawing.Size( 222, 105 );
         this.groupBoxWorldDimensions.TabIndex = 18;
         this.groupBoxWorldDimensions.TabStop = false;
         this.groupBoxWorldDimensions.Text = "World dimensions";
         // 
         // numericUpDownWorldSizeZ
         // 
         this.numericUpDownWorldSizeZ.DecimalPlaces = 1;
         this.numericUpDownWorldSizeZ.Increment = new decimal( new int[] {
            1000,
            0,
            0,
            0} );
         this.numericUpDownWorldSizeZ.Location = new System.Drawing.Point( 88, 71 );
         this.numericUpDownWorldSizeZ.Maximum = new decimal( new int[] {
            1000000,
            0,
            0,
            0} );
         this.numericUpDownWorldSizeZ.Minimum = new decimal( new int[] {
            1,
            0,
            0,
            0} );
         this.numericUpDownWorldSizeZ.Name = "numericUpDownWorldSizeZ";
         this.numericUpDownWorldSizeZ.Size = new System.Drawing.Size( 122, 20 );
         this.numericUpDownWorldSizeZ.TabIndex = 9;
         this.numericUpDownWorldSizeZ.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
         this.numericUpDownWorldSizeZ.ThousandsSeparator = true;
         this.numericUpDownWorldSizeZ.Value = new decimal( new int[] {
            1000,
            0,
            0,
            0} );
         // 
         // groupBoxDataCompression
         // 
         this.groupBoxDataCompression.Controls.Add( this.checkBoxHeightmapDXT5Compression );
         this.groupBoxDataCompression.Controls.Add( this.checkBoxOverlayImgCompression );
         this.groupBoxDataCompression.Controls.Add( this.checkBoxUseZLIBCompression );
         this.groupBoxDataCompression.Location = new System.Drawing.Point( 342, 255 );
         this.groupBoxDataCompression.Name = "groupBoxDataCompression";
         this.groupBoxDataCompression.Size = new System.Drawing.Size( 240, 99 );
         this.groupBoxDataCompression.TabIndex = 19;
         this.groupBoxDataCompression.TabStop = false;
         this.groupBoxDataCompression.Text = "Data compression";
         // 
         // groupBoxStreanableDataSettings
         // 
         this.groupBoxStreanableDataSettings.Controls.Add( this.numericUpDownOverlaymapLODoffset );
         this.groupBoxStreanableDataSettings.Controls.Add( this.numericUpDownNormalmapLODoffset );
         this.groupBoxStreanableDataSettings.Controls.Add( this.numericUpDownHeightmapLODoffset );
         this.groupBoxStreanableDataSettings.Controls.Add( this.comboBoxOverlaymapBlockSize );
         this.groupBoxStreanableDataSettings.Controls.Add( this.comboBoxNormalmapBlockSize );
         this.groupBoxStreanableDataSettings.Controls.Add( this.comboBoxHeightmapBlockSize );
         this.groupBoxStreanableDataSettings.Controls.Add( this.label18 );
         this.groupBoxStreanableDataSettings.Controls.Add( this.label15 );
         this.groupBoxStreanableDataSettings.Controls.Add( this.label11 );
         this.groupBoxStreanableDataSettings.Controls.Add( this.label17 );
         this.groupBoxStreanableDataSettings.Controls.Add( this.label14 );
         this.groupBoxStreanableDataSettings.Controls.Add( this.label10 );
         this.groupBoxStreanableDataSettings.Controls.Add( this.labelOverlayMap );
         this.groupBoxStreanableDataSettings.Controls.Add( this.labelNormalMapBlockSize );
         this.groupBoxStreanableDataSettings.Controls.Add( this.labelHeightmap );
         this.groupBoxStreanableDataSettings.Location = new System.Drawing.Point( 12, 255 );
         this.groupBoxStreanableDataSettings.Name = "groupBoxStreanableDataSettings";
         this.groupBoxStreanableDataSettings.Size = new System.Drawing.Size( 324, 99 );
         this.groupBoxStreanableDataSettings.TabIndex = 20;
         this.groupBoxStreanableDataSettings.TabStop = false;
         this.groupBoxStreanableDataSettings.Text = "Streamable data settings";
         // 
         // numericUpDownOverlaymapLODoffset
         // 
         this.numericUpDownOverlaymapLODoffset.Location = new System.Drawing.Point( 275, 71 );
         this.numericUpDownOverlaymapLODoffset.Maximum = new decimal( new int[] {
            16,
            0,
            0,
            0} );
         this.numericUpDownOverlaymapLODoffset.Name = "numericUpDownOverlaymapLODoffset";
         this.numericUpDownOverlaymapLODoffset.Size = new System.Drawing.Size( 39, 20 );
         this.numericUpDownOverlaymapLODoffset.TabIndex = 2;
         this.numericUpDownOverlaymapLODoffset.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
         this.numericUpDownOverlaymapLODoffset.Value = new decimal( new int[] {
            4,
            0,
            0,
            0} );
         // 
         // numericUpDownNormalmapLODoffset
         // 
         this.numericUpDownNormalmapLODoffset.Location = new System.Drawing.Point( 275, 46 );
         this.numericUpDownNormalmapLODoffset.Maximum = new decimal( new int[] {
            16,
            0,
            0,
            0} );
         this.numericUpDownNormalmapLODoffset.Name = "numericUpDownNormalmapLODoffset";
         this.numericUpDownNormalmapLODoffset.Size = new System.Drawing.Size( 39, 20 );
         this.numericUpDownNormalmapLODoffset.TabIndex = 2;
         this.numericUpDownNormalmapLODoffset.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
         this.numericUpDownNormalmapLODoffset.Value = new decimal( new int[] {
            4,
            0,
            0,
            0} );
         // 
         // numericUpDownHeightmapLODoffset
         // 
         this.numericUpDownHeightmapLODoffset.InterceptArrowKeys = false;
         this.numericUpDownHeightmapLODoffset.Location = new System.Drawing.Point( 275, 21 );
         this.numericUpDownHeightmapLODoffset.Maximum = new decimal( new int[] {
            16,
            0,
            0,
            0} );
         this.numericUpDownHeightmapLODoffset.Name = "numericUpDownHeightmapLODoffset";
         this.numericUpDownHeightmapLODoffset.ReadOnly = true;
         this.numericUpDownHeightmapLODoffset.Size = new System.Drawing.Size( 38, 20 );
         this.numericUpDownHeightmapLODoffset.TabIndex = 2;
         this.numericUpDownHeightmapLODoffset.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
         this.numericUpDownHeightmapLODoffset.Value = new decimal( new int[] {
            3,
            0,
            0,
            0} );
         // 
         // comboBoxOverlaymapBlockSize
         // 
         this.comboBoxOverlaymapBlockSize.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
         this.comboBoxOverlaymapBlockSize.FormattingEnabled = true;
         this.comboBoxOverlaymapBlockSize.Items.AddRange( new object[] {
            "256",
            "512",
            "1024",
            "2048"} );
         this.comboBoxOverlaymapBlockSize.Location = new System.Drawing.Point( 134, 71 );
         this.comboBoxOverlaymapBlockSize.Name = "comboBoxOverlaymapBlockSize";
         this.comboBoxOverlaymapBlockSize.Size = new System.Drawing.Size( 68, 21 );
         this.comboBoxOverlaymapBlockSize.TabIndex = 1;
         // 
         // comboBoxNormalmapBlockSize
         // 
         this.comboBoxNormalmapBlockSize.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
         this.comboBoxNormalmapBlockSize.FormattingEnabled = true;
         this.comboBoxNormalmapBlockSize.Items.AddRange( new object[] {
            "256",
            "512",
            "1024",
            "2048"} );
         this.comboBoxNormalmapBlockSize.Location = new System.Drawing.Point( 134, 46 );
         this.comboBoxNormalmapBlockSize.Name = "comboBoxNormalmapBlockSize";
         this.comboBoxNormalmapBlockSize.Size = new System.Drawing.Size( 68, 21 );
         this.comboBoxNormalmapBlockSize.TabIndex = 1;
         // 
         // comboBoxHeightmapBlockSize
         // 
         this.comboBoxHeightmapBlockSize.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
         this.comboBoxHeightmapBlockSize.FormattingEnabled = true;
         this.comboBoxHeightmapBlockSize.Items.AddRange( new object[] {
            "256",
            "512",
            "1024",
            "2048"} );
         this.comboBoxHeightmapBlockSize.Location = new System.Drawing.Point( 134, 21 );
         this.comboBoxHeightmapBlockSize.Name = "comboBoxHeightmapBlockSize";
         this.comboBoxHeightmapBlockSize.Size = new System.Drawing.Size( 68, 21 );
         this.comboBoxHeightmapBlockSize.TabIndex = 1;
         // 
         // label18
         // 
         this.label18.AutoSize = true;
         this.label18.Location = new System.Drawing.Point( 212, 74 );
         this.label18.Name = "label18";
         this.label18.Size = new System.Drawing.Size( 61, 13 );
         this.label18.TabIndex = 0;
         this.label18.Text = "LOD offset:";
         // 
         // label15
         // 
         this.label15.AutoSize = true;
         this.label15.Location = new System.Drawing.Point( 212, 49 );
         this.label15.Name = "label15";
         this.label15.Size = new System.Drawing.Size( 61, 13 );
         this.label15.TabIndex = 0;
         this.label15.Text = "LOD offset:";
         // 
         // label11
         // 
         this.label11.AutoSize = true;
         this.label11.Location = new System.Drawing.Point( 212, 24 );
         this.label11.Name = "label11";
         this.label11.Size = new System.Drawing.Size( 61, 13 );
         this.label11.TabIndex = 0;
         this.label11.Text = "LOD offset:";
         // 
         // label17
         // 
         this.label17.AutoSize = true;
         this.label17.Location = new System.Drawing.Point( 77, 74 );
         this.label17.Name = "label17";
         this.label17.Size = new System.Drawing.Size( 57, 13 );
         this.label17.TabIndex = 0;
         this.label17.Text = "block size:";
         // 
         // label14
         // 
         this.label14.AutoSize = true;
         this.label14.Location = new System.Drawing.Point( 77, 49 );
         this.label14.Name = "label14";
         this.label14.Size = new System.Drawing.Size( 57, 13 );
         this.label14.TabIndex = 0;
         this.label14.Text = "block size:";
         // 
         // label10
         // 
         this.label10.AutoSize = true;
         this.label10.Location = new System.Drawing.Point( 77, 24 );
         this.label10.Name = "label10";
         this.label10.Size = new System.Drawing.Size( 57, 13 );
         this.label10.TabIndex = 0;
         this.label10.Text = "block size:";
         // 
         // labelOverlayMap
         // 
         this.labelOverlayMap.AutoSize = true;
         this.labelOverlayMap.Location = new System.Drawing.Point( 9, 74 );
         this.labelOverlayMap.Name = "labelOverlayMap";
         this.labelOverlayMap.Size = new System.Drawing.Size( 63, 13 );
         this.labelOverlayMap.TabIndex = 0;
         this.labelOverlayMap.Text = "Overlaymap";
         // 
         // labelNormalMapBlockSize
         // 
         this.labelNormalMapBlockSize.AutoSize = true;
         this.labelNormalMapBlockSize.Location = new System.Drawing.Point( 9, 49 );
         this.labelNormalMapBlockSize.Name = "labelNormalMapBlockSize";
         this.labelNormalMapBlockSize.Size = new System.Drawing.Size( 60, 13 );
         this.labelNormalMapBlockSize.TabIndex = 0;
         this.labelNormalMapBlockSize.Text = "Normalmap";
         // 
         // labelHeightmap
         // 
         this.labelHeightmap.AutoSize = true;
         this.labelHeightmap.Location = new System.Drawing.Point( 9, 24 );
         this.labelHeightmap.Name = "labelHeightmap";
         this.labelHeightmap.Size = new System.Drawing.Size( 58, 13 );
         this.labelHeightmap.TabIndex = 0;
         this.labelHeightmap.Text = "Heightmap";
         // 
         // groupBoxLODSettings
         // 
         this.groupBoxLODSettings.Controls.Add( this.labelLODLevelDistanceRatio );
         this.groupBoxLODSettings.Controls.Add( this.labelRenderGridResolutionMultiplier );
         this.groupBoxLODSettings.Controls.Add( this.labelLeafQuadtreeNodeSize );
         this.groupBoxLODSettings.Controls.Add( this.numericUpDownLODLevelDistanceRatio );
         this.groupBoxLODSettings.Controls.Add( this.labelLODLevelCount );
         this.groupBoxLODSettings.Controls.Add( this.numericUpDownLODLevelCount );
         this.groupBoxLODSettings.Controls.Add( this.comboBoxRenderGridResolutionMultiplier );
         this.groupBoxLODSettings.Controls.Add( this.comboBoxLeafQuadtreeNodeSize );
         this.groupBoxLODSettings.Location = new System.Drawing.Point( 240, 144 );
         this.groupBoxLODSettings.Name = "groupBoxLODSettings";
         this.groupBoxLODSettings.Size = new System.Drawing.Size( 342, 105 );
         this.groupBoxLODSettings.TabIndex = 21;
         this.groupBoxLODSettings.TabStop = false;
         this.groupBoxLODSettings.Text = "LOD settings";
         // 
         // labelRenderGridResolutionMultiplier
         // 
         this.labelRenderGridResolutionMultiplier.AutoSize = true;
         this.labelRenderGridResolutionMultiplier.Location = new System.Drawing.Point( 9, 78 );
         this.labelRenderGridResolutionMultiplier.Name = "labelRenderGridResolutionMultiplier";
         this.labelRenderGridResolutionMultiplier.Size = new System.Drawing.Size( 156, 13 );
         this.labelRenderGridResolutionMultiplier.TabIndex = 0;
         this.labelRenderGridResolutionMultiplier.Text = "Render grid resolution multiplier:";
         // 
         // labelLeafQuadtreeNodeSize
         // 
         this.labelLeafQuadtreeNodeSize.AutoSize = true;
         this.labelLeafQuadtreeNodeSize.Location = new System.Drawing.Point( 9, 53 );
         this.labelLeafQuadtreeNodeSize.Name = "labelLeafQuadtreeNodeSize";
         this.labelLeafQuadtreeNodeSize.Size = new System.Drawing.Size( 124, 13 );
         this.labelLeafQuadtreeNodeSize.TabIndex = 0;
         this.labelLeafQuadtreeNodeSize.Text = "Leaf quadtree node size:";
         // 
         // labelLODLevelCount
         // 
         this.labelLODLevelCount.AutoSize = true;
         this.labelLODLevelCount.Location = new System.Drawing.Point( 9, 29 );
         this.labelLODLevelCount.Name = "labelLODLevelCount";
         this.labelLODLevelCount.Size = new System.Drawing.Size( 87, 13 );
         this.labelLODLevelCount.TabIndex = 0;
         this.labelLODLevelCount.Text = "LOD level count:";
         // 
         // numericUpDownLODLevelCount
         // 
         this.numericUpDownLODLevelCount.Location = new System.Drawing.Point( 96, 25 );
         this.numericUpDownLODLevelCount.Maximum = new decimal( new int[] {
            15,
            0,
            0,
            0} );
         this.numericUpDownLODLevelCount.Minimum = new decimal( new int[] {
            2,
            0,
            0,
            0} );
         this.numericUpDownLODLevelCount.Name = "numericUpDownLODLevelCount";
         this.numericUpDownLODLevelCount.Size = new System.Drawing.Size( 52, 20 );
         this.numericUpDownLODLevelCount.TabIndex = 2;
         this.numericUpDownLODLevelCount.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
         this.numericUpDownLODLevelCount.Value = new decimal( new int[] {
            2,
            0,
            0,
            0} );
         // 
         // comboBoxRenderGridResolutionMultiplier
         // 
         this.comboBoxRenderGridResolutionMultiplier.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
         this.comboBoxRenderGridResolutionMultiplier.FormattingEnabled = true;
         this.comboBoxRenderGridResolutionMultiplier.Items.AddRange( new object[] {
            "2",
            "4",
            "8",
            "16",
            "32",
            "64"} );
         this.comboBoxRenderGridResolutionMultiplier.Location = new System.Drawing.Point( 193, 73 );
         this.comboBoxRenderGridResolutionMultiplier.Name = "comboBoxRenderGridResolutionMultiplier";
         this.comboBoxRenderGridResolutionMultiplier.Size = new System.Drawing.Size( 68, 21 );
         this.comboBoxRenderGridResolutionMultiplier.TabIndex = 1;
         this.comboBoxRenderGridResolutionMultiplier.SelectedIndexChanged += new System.EventHandler( this.comboBoxRenderGridResolutionMultiplier_SelectedIndexChanged );
         // 
         // comboBoxLeafQuadtreeNodeSize
         // 
         this.comboBoxLeafQuadtreeNodeSize.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
         this.comboBoxLeafQuadtreeNodeSize.FormattingEnabled = true;
         this.comboBoxLeafQuadtreeNodeSize.Items.AddRange( new object[] {
            "2",
            "4",
            "8",
            "16",
            "32",
            "64"} );
         this.comboBoxLeafQuadtreeNodeSize.Location = new System.Drawing.Point( 193, 49 );
         this.comboBoxLeafQuadtreeNodeSize.Name = "comboBoxLeafQuadtreeNodeSize";
         this.comboBoxLeafQuadtreeNodeSize.Size = new System.Drawing.Size( 68, 21 );
         this.comboBoxLeafQuadtreeNodeSize.TabIndex = 1;
         // 
         // groupBoxRendaring
         // 
         this.groupBoxRendaring.Controls.Add( this.labelMaxViewRange );
         this.groupBoxRendaring.Controls.Add( this.labelMinViewRange );
         this.groupBoxRendaring.Controls.Add( this.numericUpDownMaxViewRange );
         this.groupBoxRendaring.Controls.Add( this.numericUpDownMinViewRange );
         this.groupBoxRendaring.Location = new System.Drawing.Point( 12, 361 );
         this.groupBoxRendaring.Name = "groupBoxRendaring";
         this.groupBoxRendaring.Size = new System.Drawing.Size( 222, 96 );
         this.groupBoxRendaring.TabIndex = 22;
         this.groupBoxRendaring.TabStop = false;
         this.groupBoxRendaring.Text = "Rendering";
         // 
         // labelLODLevelDistanceRatio
         // 
         this.labelLODLevelDistanceRatio.AutoSize = true;
         this.labelLODLevelDistanceRatio.Location = new System.Drawing.Point( 159, 29 );
         this.labelLODLevelDistanceRatio.Name = "labelLODLevelDistanceRatio";
         this.labelLODLevelDistanceRatio.Size = new System.Drawing.Size( 123, 13 );
         this.labelLODLevelDistanceRatio.TabIndex = 0;
         this.labelLODLevelDistanceRatio.Text = "LOD level distance ratio:";
         // 
         // labelMaxViewRange
         // 
         this.labelMaxViewRange.AutoSize = true;
         this.labelMaxViewRange.Location = new System.Drawing.Point( 9, 48 );
         this.labelMaxViewRange.Name = "labelMaxViewRange";
         this.labelMaxViewRange.Size = new System.Drawing.Size( 109, 13 );
         this.labelMaxViewRange.TabIndex = 0;
         this.labelMaxViewRange.Text = "Maximum view range:";
         // 
         // labelMinViewRange
         // 
         this.labelMinViewRange.AutoSize = true;
         this.labelMinViewRange.Location = new System.Drawing.Point( 9, 25 );
         this.labelMinViewRange.Name = "labelMinViewRange";
         this.labelMinViewRange.Size = new System.Drawing.Size( 106, 13 );
         this.labelMinViewRange.TabIndex = 0;
         this.labelMinViewRange.Text = "Minimum view range:";
         // 
         // numericUpDownLODLevelDistanceRatio
         // 
         this.numericUpDownLODLevelDistanceRatio.DecimalPlaces = 2;
         this.numericUpDownLODLevelDistanceRatio.Increment = new decimal( new int[] {
            1,
            0,
            0,
            65536} );
         this.numericUpDownLODLevelDistanceRatio.Location = new System.Drawing.Point( 283, 25 );
         this.numericUpDownLODLevelDistanceRatio.Maximum = new decimal( new int[] {
            10,
            0,
            0,
            0} );
         this.numericUpDownLODLevelDistanceRatio.Name = "numericUpDownLODLevelDistanceRatio";
         this.numericUpDownLODLevelDistanceRatio.Size = new System.Drawing.Size( 52, 20 );
         this.numericUpDownLODLevelDistanceRatio.TabIndex = 7;
         this.numericUpDownLODLevelDistanceRatio.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
         this.numericUpDownLODLevelDistanceRatio.ThousandsSeparator = true;
         this.numericUpDownLODLevelDistanceRatio.Value = new decimal( new int[] {
            2,
            0,
            0,
            0} );
         // 
         // numericUpDownMaxViewRange
         // 
         this.numericUpDownMaxViewRange.DecimalPlaces = 1;
         this.numericUpDownMaxViewRange.Increment = new decimal( new int[] {
            10,
            0,
            0,
            0} );
         this.numericUpDownMaxViewRange.Location = new System.Drawing.Point( 131, 46 );
         this.numericUpDownMaxViewRange.Maximum = new decimal( new int[] {
            1000000,
            0,
            0,
            0} );
         this.numericUpDownMaxViewRange.Name = "numericUpDownMaxViewRange";
         this.numericUpDownMaxViewRange.Size = new System.Drawing.Size( 81, 20 );
         this.numericUpDownMaxViewRange.TabIndex = 7;
         this.numericUpDownMaxViewRange.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
         this.numericUpDownMaxViewRange.ThousandsSeparator = true;
         this.numericUpDownMaxViewRange.Value = new decimal( new int[] {
            10000,
            0,
            0,
            0} );
         // 
         // numericUpDownMinViewRange
         // 
         this.numericUpDownMinViewRange.DecimalPlaces = 1;
         this.numericUpDownMinViewRange.Increment = new decimal( new int[] {
            10,
            0,
            0,
            0} );
         this.numericUpDownMinViewRange.Location = new System.Drawing.Point( 131, 23 );
         this.numericUpDownMinViewRange.Maximum = new decimal( new int[] {
            1000000,
            0,
            0,
            0} );
         this.numericUpDownMinViewRange.Name = "numericUpDownMinViewRange";
         this.numericUpDownMinViewRange.Size = new System.Drawing.Size( 81, 20 );
         this.numericUpDownMinViewRange.TabIndex = 7;
         this.numericUpDownMinViewRange.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
         this.numericUpDownMinViewRange.ThousandsSeparator = true;
         this.numericUpDownMinViewRange.Value = new decimal( new int[] {
            10000,
            0,
            0,
            0} );
         // 
         // groupBoxDetailMap
         // 
         this.groupBoxDetailMap.Controls.Add( this.checkBoxDetailMapEnabled );
         this.groupBoxDetailMap.Controls.Add( this.numericUpDownDetailMapSizeZ );
         this.groupBoxDetailMap.Controls.Add( this.numericUpDownDetailMapLODLevelsAffected );
         this.groupBoxDetailMap.Controls.Add( this.numericUpDownDetailMapSizeX );
         this.groupBoxDetailMap.Controls.Add( this.numericUpDownDetailMapSizeY );
         this.groupBoxDetailMap.Controls.Add( this.label29 );
         this.groupBoxDetailMap.Controls.Add( this.label28 );
         this.groupBoxDetailMap.Controls.Add( this.label25 );
         this.groupBoxDetailMap.Controls.Add( this.label26 );
         this.groupBoxDetailMap.Location = new System.Drawing.Point( 252, 361 );
         this.groupBoxDetailMap.Name = "groupBoxDetailMap";
         this.groupBoxDetailMap.Size = new System.Drawing.Size( 330, 96 );
         this.groupBoxDetailMap.TabIndex = 22;
         this.groupBoxDetailMap.TabStop = false;
         this.groupBoxDetailMap.Text = "Detail map";
         // 
         // checkBoxDetailMapEnabled
         // 
         this.checkBoxDetailMapEnabled.AutoSize = true;
         this.checkBoxDetailMapEnabled.Checked = true;
         this.checkBoxDetailMapEnabled.CheckState = System.Windows.Forms.CheckState.Checked;
         this.checkBoxDetailMapEnabled.Location = new System.Drawing.Point( 12, 71 );
         this.checkBoxDetailMapEnabled.Name = "checkBoxDetailMapEnabled";
         this.checkBoxDetailMapEnabled.Size = new System.Drawing.Size( 65, 17 );
         this.checkBoxDetailMapEnabled.TabIndex = 1;
         this.checkBoxDetailMapEnabled.Text = "Enabled";
         this.checkBoxDetailMapEnabled.UseVisualStyleBackColor = true;
         // 
         // numericUpDownDetailMapSizeZ
         // 
         this.numericUpDownDetailMapSizeZ.DecimalPlaces = 1;
         this.numericUpDownDetailMapSizeZ.Location = new System.Drawing.Point( 212, 22 );
         this.numericUpDownDetailMapSizeZ.Maximum = new decimal( new int[] {
            10000,
            0,
            0,
            0} );
         this.numericUpDownDetailMapSizeZ.Name = "numericUpDownDetailMapSizeZ";
         this.numericUpDownDetailMapSizeZ.Size = new System.Drawing.Size( 81, 20 );
         this.numericUpDownDetailMapSizeZ.TabIndex = 7;
         this.numericUpDownDetailMapSizeZ.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
         this.numericUpDownDetailMapSizeZ.ThousandsSeparator = true;
         this.numericUpDownDetailMapSizeZ.Value = new decimal( new int[] {
            6,
            0,
            0,
            0} );
         // 
         // numericUpDownDetailMapLODLevelsAffected
         // 
         this.numericUpDownDetailMapLODLevelsAffected.Location = new System.Drawing.Point( 253, 45 );
         this.numericUpDownDetailMapLODLevelsAffected.Maximum = new decimal( new int[] {
            16,
            0,
            0,
            0} );
         this.numericUpDownDetailMapLODLevelsAffected.Name = "numericUpDownDetailMapLODLevelsAffected";
         this.numericUpDownDetailMapLODLevelsAffected.Size = new System.Drawing.Size( 40, 20 );
         this.numericUpDownDetailMapLODLevelsAffected.TabIndex = 2;
         this.numericUpDownDetailMapLODLevelsAffected.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
         this.numericUpDownDetailMapLODLevelsAffected.Value = new decimal( new int[] {
            4,
            0,
            0,
            0} );
         // 
         // numericUpDownDetailMapSizeX
         // 
         this.numericUpDownDetailMapSizeX.DecimalPlaces = 1;
         this.numericUpDownDetailMapSizeX.Increment = new decimal( new int[] {
            10,
            0,
            0,
            0} );
         this.numericUpDownDetailMapSizeX.Location = new System.Drawing.Point( 52, 22 );
         this.numericUpDownDetailMapSizeX.Maximum = new decimal( new int[] {
            10000,
            0,
            0,
            0} );
         this.numericUpDownDetailMapSizeX.Name = "numericUpDownDetailMapSizeX";
         this.numericUpDownDetailMapSizeX.Size = new System.Drawing.Size( 81, 20 );
         this.numericUpDownDetailMapSizeX.TabIndex = 7;
         this.numericUpDownDetailMapSizeX.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
         this.numericUpDownDetailMapSizeX.ThousandsSeparator = true;
         this.numericUpDownDetailMapSizeX.Value = new decimal( new int[] {
            200,
            0,
            0,
            0} );
         // 
         // numericUpDownDetailMapSizeY
         // 
         this.numericUpDownDetailMapSizeY.DecimalPlaces = 1;
         this.numericUpDownDetailMapSizeY.Increment = new decimal( new int[] {
            10,
            0,
            0,
            0} );
         this.numericUpDownDetailMapSizeY.Location = new System.Drawing.Point( 52, 45 );
         this.numericUpDownDetailMapSizeY.Maximum = new decimal( new int[] {
            10000,
            0,
            0,
            0} );
         this.numericUpDownDetailMapSizeY.Name = "numericUpDownDetailMapSizeY";
         this.numericUpDownDetailMapSizeY.Size = new System.Drawing.Size( 81, 20 );
         this.numericUpDownDetailMapSizeY.TabIndex = 7;
         this.numericUpDownDetailMapSizeY.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
         this.numericUpDownDetailMapSizeY.ThousandsSeparator = true;
         this.numericUpDownDetailMapSizeY.Value = new decimal( new int[] {
            200,
            0,
            0,
            0} );
         // 
         // label29
         // 
         this.label29.AutoSize = true;
         this.label29.Location = new System.Drawing.Point( 151, 48 );
         this.label29.Name = "label29";
         this.label29.Size = new System.Drawing.Size( 104, 13 );
         this.label29.TabIndex = 0;
         this.label29.Text = "LOD levels affected:";
         // 
         // label28
         // 
         this.label28.AutoSize = true;
         this.label28.Location = new System.Drawing.Point( 151, 25 );
         this.label28.Name = "label28";
         this.label28.Size = new System.Drawing.Size( 40, 13 );
         this.label28.TabIndex = 0;
         this.label28.Text = "Size Z:";
         // 
         // label25
         // 
         this.label25.AutoSize = true;
         this.label25.Location = new System.Drawing.Point( 9, 45 );
         this.label25.Name = "label25";
         this.label25.Size = new System.Drawing.Size( 40, 13 );
         this.label25.TabIndex = 0;
         this.label25.Text = "Size Y:";
         // 
         // label26
         // 
         this.label26.AutoSize = true;
         this.label26.Location = new System.Drawing.Point( 9, 25 );
         this.label26.Name = "label26";
         this.label26.Size = new System.Drawing.Size( 37, 13 );
         this.label26.TabIndex = 0;
         this.label26.Text = "Size X";
         // 
         // WizardForm
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF( 6F, 13F );
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.ClientSize = new System.Drawing.Size( 594, 639 );
         this.Controls.Add( this.groupBoxDetailMap );
         this.Controls.Add( this.groupBoxRendaring );
         this.Controls.Add( this.groupBoxLODSettings );
         this.Controls.Add( this.groupBoxStreanableDataSettings );
         this.Controls.Add( this.groupBoxDataCompression );
         this.Controls.Add( this.groupBoxWorldDimensions );
         this.Controls.Add( this.groupBoxSourceData );
         this.Controls.Add( this.buttonGo );
         this.Controls.Add( this.richTextBoxStats );
         this.Controls.Add( this.labelStats );
         this.Controls.Add( this.buttonBrowseOutputSDLOD );
         this.Controls.Add( this.textBoxOutputSDLOD );
         this.Controls.Add( this.label7 );
         this.Controls.Add( this.menuStrip1 );
         this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
         this.MainMenuStrip = this.menuStrip1;
         this.MaximizeBox = false;
         this.MinimizeBox = false;
         this.Name = "WizardForm";
         this.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Hide;
         this.Text = "DEMO Streamable DLOD Terrain Creation Wizard";
         this.Load += new System.EventHandler( this.WizardForm_Load );
         this.menuStrip1.ResumeLayout( false );
         this.menuStrip1.PerformLayout( );
         ( (System.ComponentModel.ISupportInitialize)( this.numericUpDownWorldSizeX ) ).EndInit( );
         ( (System.ComponentModel.ISupportInitialize)( this.numericUpDownWorldSizeY ) ).EndInit( );
         this.groupBoxSourceData.ResumeLayout( false );
         this.groupBoxSourceData.PerformLayout( );
         this.groupBoxWorldDimensions.ResumeLayout( false );
         this.groupBoxWorldDimensions.PerformLayout( );
         ( (System.ComponentModel.ISupportInitialize)( this.numericUpDownWorldSizeZ ) ).EndInit( );
         this.groupBoxDataCompression.ResumeLayout( false );
         this.groupBoxDataCompression.PerformLayout( );
         this.groupBoxStreanableDataSettings.ResumeLayout( false );
         this.groupBoxStreanableDataSettings.PerformLayout( );
         ( (System.ComponentModel.ISupportInitialize)( this.numericUpDownOverlaymapLODoffset ) ).EndInit( );
         ( (System.ComponentModel.ISupportInitialize)( this.numericUpDownNormalmapLODoffset ) ).EndInit( );
         ( (System.ComponentModel.ISupportInitialize)( this.numericUpDownHeightmapLODoffset ) ).EndInit( );
         this.groupBoxLODSettings.ResumeLayout( false );
         this.groupBoxLODSettings.PerformLayout( );
         ( (System.ComponentModel.ISupportInitialize)( this.numericUpDownLODLevelCount ) ).EndInit( );
         this.groupBoxRendaring.ResumeLayout( false );
         this.groupBoxRendaring.PerformLayout( );
         ( (System.ComponentModel.ISupportInitialize)( this.numericUpDownLODLevelDistanceRatio ) ).EndInit( );
         ( (System.ComponentModel.ISupportInitialize)( this.numericUpDownMaxViewRange ) ).EndInit( );
         ( (System.ComponentModel.ISupportInitialize)( this.numericUpDownMinViewRange ) ).EndInit( );
         this.groupBoxDetailMap.ResumeLayout( false );
         this.groupBoxDetailMap.PerformLayout( );
         ( (System.ComponentModel.ISupportInitialize)( this.numericUpDownDetailMapSizeZ ) ).EndInit( );
         ( (System.ComponentModel.ISupportInitialize)( this.numericUpDownDetailMapLODLevelsAffected ) ).EndInit( );
         ( (System.ComponentModel.ISupportInitialize)( this.numericUpDownDetailMapSizeX ) ).EndInit( );
         ( (System.ComponentModel.ISupportInitialize)( this.numericUpDownDetailMapSizeY ) ).EndInit( );
         this.ResumeLayout( false );
         this.PerformLayout( );

      }

      #endregion

      private System.Windows.Forms.MenuStrip menuStrip1;
      private System.Windows.Forms.ToolStripMenuItem fileToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem convertTBMPToolStripMenuItem;
      private System.Windows.Forms.Label labelHeightmapFile;
      private System.Windows.Forms.TextBox textBoxHeightmap;
      private System.Windows.Forms.Button buttonBrowseHeightmap;
      private System.Windows.Forms.Label labelWorldX;
      private System.Windows.Forms.NumericUpDown numericUpDownWorldSizeX;
      private System.Windows.Forms.NumericUpDown numericUpDownWorldSizeY;
      private System.Windows.Forms.Label labelWorldY;
      private System.Windows.Forms.Label labelWorldZ;
      private System.Windows.Forms.Label labelStats;
      private System.Windows.Forms.RichTextBox richTextBoxStats;
      private System.Windows.Forms.Label labelOverlayImage;
      private System.Windows.Forms.TextBox textBoxOverlayImage;
      private System.Windows.Forms.Button buttonBrowseOverlay;
      private System.Windows.Forms.Label label7;
      private System.Windows.Forms.TextBox textBoxOutputSDLOD;
      private System.Windows.Forms.Button buttonBrowseOutputSDLOD;
      private System.Windows.Forms.Button buttonGo;
      private System.Windows.Forms.CheckBox checkBoxOverlayImgCompression;
      private System.Windows.Forms.CheckBox checkBoxHeightmapDXT5Compression;
      private System.Windows.Forms.CheckBox checkBoxUseZLIBCompression;
      private System.Windows.Forms.GroupBox groupBoxSourceData;
      private System.Windows.Forms.Label labelNormalMap;
      private System.Windows.Forms.GroupBox groupBoxWorldDimensions;
      private System.Windows.Forms.GroupBox groupBoxDataCompression;
      private System.Windows.Forms.GroupBox groupBoxStreanableDataSettings;
      private System.Windows.Forms.ComboBox comboBoxHeightmapBlockSize;
      private System.Windows.Forms.Label labelHeightmap;
      private System.Windows.Forms.NumericUpDown numericUpDownHeightmapLODoffset;
      private System.Windows.Forms.Label label11;
      private System.Windows.Forms.Label label10;
      private System.Windows.Forms.Label labelNormalMapExplanation;
      private System.Windows.Forms.NumericUpDown numericUpDownWorldSizeZ;
      private System.Windows.Forms.NumericUpDown numericUpDownOverlaymapLODoffset;
      private System.Windows.Forms.NumericUpDown numericUpDownNormalmapLODoffset;
      private System.Windows.Forms.ComboBox comboBoxOverlaymapBlockSize;
      private System.Windows.Forms.ComboBox comboBoxNormalmapBlockSize;
      private System.Windows.Forms.Label label18;
      private System.Windows.Forms.Label label15;
      private System.Windows.Forms.Label label17;
      private System.Windows.Forms.Label label14;
      private System.Windows.Forms.Label labelOverlayMap;
      private System.Windows.Forms.Label labelNormalMapBlockSize;
      private System.Windows.Forms.GroupBox groupBoxLODSettings;
      private System.Windows.Forms.Label labelLeafQuadtreeNodeSize;
      private System.Windows.Forms.Label labelLODLevelCount;
      private System.Windows.Forms.NumericUpDown numericUpDownLODLevelCount;
      private System.Windows.Forms.Label labelRenderGridResolutionMultiplier;
      private System.Windows.Forms.GroupBox groupBoxRendaring;
      private System.Windows.Forms.Label labelLODLevelDistanceRatio;
      private System.Windows.Forms.Label labelMaxViewRange;
      private System.Windows.Forms.Label labelMinViewRange;
      private System.Windows.Forms.GroupBox groupBoxDetailMap;
      private System.Windows.Forms.CheckBox checkBoxDetailMapEnabled;
      private System.Windows.Forms.Label label29;
      private System.Windows.Forms.Label label28;
      private System.Windows.Forms.Label label25;
      private System.Windows.Forms.Label label26;
      private System.Windows.Forms.NumericUpDown numericUpDownLODLevelDistanceRatio;
      private System.Windows.Forms.NumericUpDown numericUpDownMaxViewRange;
      private System.Windows.Forms.NumericUpDown numericUpDownMinViewRange;
      private System.Windows.Forms.NumericUpDown numericUpDownDetailMapSizeZ;
      private System.Windows.Forms.NumericUpDown numericUpDownDetailMapLODLevelsAffected;
      private System.Windows.Forms.NumericUpDown numericUpDownDetailMapSizeX;
      private System.Windows.Forms.NumericUpDown numericUpDownDetailMapSizeY;
      private System.Windows.Forms.ComboBox comboBoxLeafQuadtreeNodeSize;
      private System.Windows.Forms.ComboBox comboBoxRenderGridResolutionMultiplier;
      private System.Windows.Forms.ToolStripMenuItem openToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem saveAsToolStripMenuItem;
      private System.Windows.Forms.ToolStripSeparator toolStripMenuItem1;
      private System.Windows.Forms.Button buttonClearOverlaymapPath;

   }
}

