using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using System.IO;

using Affine;

namespace Wizard
{
   public partial class WizardForm : Form
   {
      public WizardForm( )
      {
         InitializeComponent( );
      }

      private void WizardForm_Load( object sender, EventArgs e )
      {
         comboBoxLeafQuadtreeNodeSize.SelectedIndex = 2;
         comboBoxRenderGridResolutionMultiplier.SelectedIndex = 2;

         comboBoxHeightmapBlockSize.SelectedIndex = 1;
         comboBoxNormalmapBlockSize.SelectedIndex = 2;
         comboBoxOverlaymapBlockSize.SelectedIndex = 2;
      }

      private void buttonBrowseHeightmap_Click( object sender, EventArgs e )
      {
         OpenFileDialog openFileDialog = new OpenFileDialog( );
         openFileDialog.Filter = "Tiled bitmap (.tbmp) |*.tbmp";
         openFileDialog.Title = "Open Heightmap";

         if( openFileDialog.ShowDialog( ) == DialogResult.OK )
         {
            TiledBitmap heightmap = TiledBitmap.Open( openFileDialog.FileName, true );

            if( heightmap.PixelFormat != TiledBitmapPixelFormat.Format16BitGrayScale )
            {
               MessageBox.Show( "Heightmap format must be 16 bit grayscale." );
            }
            else
            {
               // Set path into heightmap text box
               textBoxHeightmap.Text = openFileDialog.FileName;
               SetupInitialParameters( heightmap.Width, heightmap.Height );
            }

            heightmap.Close( );
         }
      }

      private void SetupInitialParameters( int heightmapWidth, int heightmapHeight )
      {
         // world dimensions
         numericUpDownWorldSizeX.Value = (decimal)( 10.0f * ( heightmapWidth - 1 ) );
         numericUpDownWorldSizeY.Value = (decimal)( 10.0f * ( heightmapHeight - 1 ) );
         numericUpDownWorldSizeZ.Value = (decimal)100.0f;

         // LOD settings
         // calculate approx LOD level 
         {
            int maxDim = Math.Max( heightmapWidth - 1, heightmapHeight - 1 );
            int temp = (int)Math.Ceiling( Math.Log( maxDim, 2 ) );

            numericUpDownLODLevelCount.Value = Math.Max( 2, Math.Min( 15, temp - 4 ) );
         }
         comboBoxLeafQuadtreeNodeSize.SelectedIndex = 2;
         comboBoxRenderGridResolutionMultiplier.SelectedIndex = 2;

         // streamable data settings
         comboBoxHeightmapBlockSize.SelectedIndex = 1;
         comboBoxNormalmapBlockSize.SelectedIndex = 2;
         comboBoxOverlaymapBlockSize.SelectedIndex = 2;

         // rendering         
         float levelDistanceRatio = 2.0f;
         // this is default for numericUpDownLODLevelDistanceRatio
         float total = 0;
         float currentDetailBalance = 1.0f;
         int LODLevelCount = (int)numericUpDownLODLevelCount.Value;
         for( int i = 0; i < LODLevelCount; i++ )
         {
            total += currentDetailBalance;
            currentDetailBalance *= levelDistanceRatio;
         }
         int LeafRenderNodeSize = Convert.ToInt32( comboBoxLeafQuadtreeNodeSize.Text );
         float leafNodeWorldSizeX = LeafRenderNodeSize * (float)numericUpDownWorldSizeX.Value / (float)( heightmapWidth - 1 );
         float leafNodeWorldSizeY = LeafRenderNodeSize * (float)numericUpDownWorldSizeY.Value / (float)( heightmapHeight - 1 );
         // set min/max view range
         numericUpDownMinViewRange.Value = (decimal)( Math.Max( leafNodeWorldSizeX, leafNodeWorldSizeY ) * total * 2 );
         numericUpDownMaxViewRange.Value = 10 * numericUpDownMinViewRange.Value;

      }

      private void buttonBrowseOverlay_Click( object sender, EventArgs e )
      {
         OpenFileDialog openFileDialog = new OpenFileDialog( );
         openFileDialog.Filter = "Tiled bitmap (.tbmp) |*.tbmp";
         openFileDialog.Title = "Open Overlay Image";

         if( openFileDialog.ShowDialog( ) == DialogResult.OK )
         {
            TiledBitmap overlaymap = TiledBitmap.Open( openFileDialog.FileName, true );

            if( overlaymap.PixelFormat != TiledBitmapPixelFormat.Format24BitRGB )
            {
               MessageBox.Show( "Overlay image format must be 24 bit RGB." );
            }
            else
            {
               // Set path into overlay image text box
               textBoxOverlayImage.Text = openFileDialog.FileName;
            }

            overlaymap.Close( );
         }
      }

      private void buttonBrowseOutputSDLOD_Click( object sender, EventArgs e )
      {
         // Displays a SaveFileDialog 
         SaveFileDialog saveFileDialog = new SaveFileDialog( );
         saveFileDialog.Filter = "Streamable DLOD (.sdlod)|*.sdlod";
         saveFileDialog.FileName = "Untitled.sdlod";
         saveFileDialog.Title = "Set output file";
         // if save button has been pressed ...
         if( saveFileDialog.ShowDialog( ) == DialogResult.OK )
         {
            // ... check that we have valid name ...
            if( saveFileDialog.FileName != "" )
            {
               // ... and return path to be set into textBoxFilePath
               textBoxOutputSDLOD.Text = saveFileDialog.FileName;
            }
         }
      }

      private void comboBoxRenderGridResolutionMultiplier_SelectedIndexChanged( object sender, EventArgs e )
      {
         if( comboBoxRenderGridResolutionMultiplier.SelectedItem == null ) return;

         string value = comboBoxRenderGridResolutionMultiplier.SelectedItem.ToString( );
         if( value == "" ) return;
         int temp = Convert.ToInt32( value );
         numericUpDownHeightmapLODoffset.Value = (int)Math.Ceiling( Math.Log( temp, 2 ) );
      }

      // Records all parameters from the wizard form into a file
      private void saveAsToolStripMenuItem_Click( object sender, EventArgs e )
      {
         // Rise SaveAs dialog
         SaveFileDialog saveFileDialog = new SaveFileDialog( );
         saveFileDialog.Filter = "SDLOD Wizard Project (.swp) | *swp";
         saveFileDialog.FileName = "Untitled.swp";
         saveFileDialog.Title = "Save As";
         if( saveFileDialog.ShowDialog( ) == DialogResult.OK )
         {
            // create file
            XmlTextWriter xmlWriter = new XmlTextWriter( saveFileDialog.FileName, Encoding.Default );
            xmlWriter.Formatting = Formatting.Indented;

            xmlWriter.WriteStartElement( "Root" );

            int ver = 1;
            xmlWriter.WriteElementString( "Version", XmlConvert.ToString( ver ) );

            xmlWriter.WriteStartElement( "SourceData" );
            xmlWriter.WriteElementString( "Heightmap", textBoxHeightmap.Text );
            xmlWriter.WriteElementString( "Overlaymap", textBoxOverlayImage.Text );
            xmlWriter.WriteEndElement( );

            xmlWriter.WriteStartElement( "WorldSize" );
            xmlWriter.WriteElementString( "X", numericUpDownWorldSizeX.Value.ToString( ) );
            xmlWriter.WriteElementString( "Y", numericUpDownWorldSizeY.Value.ToString( ) );
            xmlWriter.WriteElementString( "Z", numericUpDownWorldSizeZ.Value.ToString( ) );
            xmlWriter.WriteEndElement( );

            xmlWriter.WriteStartElement( "LODSettings" );
            xmlWriter.WriteElementString( "LODLevelCount", numericUpDownLODLevelCount.Value.ToString( ) );
            xmlWriter.WriteElementString( "LeafQuadtreeNodeSize", comboBoxLeafQuadtreeNodeSize.Text );
            xmlWriter.WriteElementString( "RenderGridResolutionMultiplier", comboBoxRenderGridResolutionMultiplier.Text );
            xmlWriter.WriteElementString( "LODLevelDistanceRatio", numericUpDownLODLevelDistanceRatio.Value.ToString( ) );
            xmlWriter.WriteEndElement( );

            xmlWriter.WriteStartElement( "StreamableDataSettings" );
            xmlWriter.WriteStartElement( "BlockSize" );
            xmlWriter.WriteElementString( "Heightmap", comboBoxHeightmapBlockSize.Text );
            xmlWriter.WriteElementString( "Normalmap", comboBoxNormalmapBlockSize.Text );
            xmlWriter.WriteElementString( "Overlaymap", comboBoxOverlaymapBlockSize.Text );
            xmlWriter.WriteEndElement( );
            xmlWriter.WriteStartElement( "LODoffset" );
            xmlWriter.WriteElementString( "Heightmap", numericUpDownHeightmapLODoffset.Value.ToString( ) );
            xmlWriter.WriteElementString( "Normalmap", numericUpDownNormalmapLODoffset.Value.ToString( ) );
            xmlWriter.WriteElementString( "Overlaymap", numericUpDownOverlaymapLODoffset.Value.ToString( ) );
            xmlWriter.WriteEndElement( );
            xmlWriter.WriteEndElement( );

            xmlWriter.WriteStartElement( "DataCompression" );
            xmlWriter.WriteElementString( "HeightmapDXT5Compression", checkBoxHeightmapDXT5Compression.Checked.ToString( ) );
            xmlWriter.WriteElementString( "OverlayImgCompression", checkBoxOverlayImgCompression.Checked.ToString( ) );
            xmlWriter.WriteElementString( "UseZLIBCompression", checkBoxUseZLIBCompression.Checked.ToString( ) );
            xmlWriter.WriteEndElement( );

            xmlWriter.WriteStartElement( "Rendering" );
            xmlWriter.WriteElementString( "MinViewRange", numericUpDownMinViewRange.Value.ToString( ) );
            xmlWriter.WriteElementString( "MaxViewRange", numericUpDownMaxViewRange.Value.ToString( ) );
            xmlWriter.WriteEndElement( );

            xmlWriter.WriteStartElement( "DetailMap" );
            xmlWriter.WriteElementString( "X", numericUpDownDetailMapSizeX.Value.ToString( ) );
            xmlWriter.WriteElementString( "Y", numericUpDownDetailMapSizeY.Value.ToString( ) );
            xmlWriter.WriteElementString( "Z", numericUpDownDetailMapSizeZ.Value.ToString( ) );
            xmlWriter.WriteElementString( "DetailMapLODLevelsAffected", numericUpDownDetailMapLODLevelsAffected.Value.ToString( ) );
            xmlWriter.WriteElementString( "DetailMapEnabled", checkBoxDetailMapEnabled.Checked.ToString( ) );
            xmlWriter.WriteEndElement( );

            xmlWriter.WriteEndElement( ); // end root element
            xmlWriter.Close( );
         }
      }

      // Opens swp file, reads in parameters and sets their values on the wizard form
      private void openToolStripMenuItem_Click( object sender, EventArgs e )
      {
         OpenFileDialog openFileDialog = new OpenFileDialog( );
         openFileDialog.Filter = "SDLOD Wizard Project (.swp) | *swp";
         openFileDialog.Title = "Open SDLOD Wizard Project File";

         if( openFileDialog.ShowDialog( ) == DialogResult.OK )
         {
            XmlTextReader xmlTextReader = new XmlTextReader( openFileDialog.FileName );

            while( xmlTextReader.Read( ) )
            {
               if( xmlTextReader.NodeType != XmlNodeType.Element ) continue;

               // read and set values for SourceData group
               if( xmlTextReader.Name == "SourceData" )
               {
                  XmlReader xmlSubReader = xmlTextReader.ReadSubtree( );
                  while( xmlSubReader.Read( ) )
                  {
                     if( xmlSubReader.NodeType != XmlNodeType.Element ) continue;

                     if( xmlTextReader.Name == "Heightmap" )
                        textBoxHeightmap.Text = xmlTextReader.ReadElementString( "Heightmap" );

                     if( xmlTextReader.Name == "Overlaymap" )
                        textBoxOverlayImage.Text = xmlTextReader.ReadElementString( "Overlaymap" );
                  }
                  xmlSubReader.Close( );
               }

               // read and set values for WorldSize group
               if( xmlTextReader.Name == "WorldSize" )
               {
                  XmlReader xmlSubReader = xmlTextReader.ReadSubtree( );
                  while( xmlSubReader.Read( ) )
                  {
                     if( xmlSubReader.NodeType != XmlNodeType.Element ) continue;

                     if( xmlTextReader.Name == "X" )
                        numericUpDownWorldSizeX.Value = xmlTextReader.ReadElementContentAsDecimal( );

                     if( xmlTextReader.Name == "Y" )
                        numericUpDownWorldSizeY.Value = xmlTextReader.ReadElementContentAsDecimal( );

                     if( xmlTextReader.Name == "Z" )
                        numericUpDownWorldSizeZ.Value = xmlTextReader.ReadElementContentAsDecimal( );
                  }
                  xmlSubReader.Close( );
               }

               // read and set values for LODSettings group
               if( xmlTextReader.Name == "LODSettings" )
               {
                  XmlReader xmlSubReader = xmlTextReader.ReadSubtree( );
                  while( xmlSubReader.Read( ) )
                  {
                     if( xmlSubReader.NodeType != XmlNodeType.Element ) continue;

                     if( xmlTextReader.Name == "LODLevelCount" )
                        numericUpDownLODLevelCount.Value = xmlTextReader.ReadElementContentAsDecimal( );

                     if( xmlTextReader.Name == "LeafQuadtreeNodeSize" )
                        comboBoxLeafQuadtreeNodeSize.Text = xmlTextReader.ReadElementString( "LeafQuadtreeNodeSize" );

                     if( xmlTextReader.Name == "RenderGridResolutionMultiplier" )
                        comboBoxRenderGridResolutionMultiplier.Text = xmlTextReader.ReadElementString( "RenderGridResolutionMultiplier" );
                  }
                  xmlSubReader.Close( );
               }

               // read and set values for StreamableDataSettings group
               if( xmlTextReader.Name == "StreamableDataSettings" )
               {
                  XmlReader xmlSubReader = xmlTextReader.ReadSubtree( );
                  while( xmlSubReader.Read( ) )
                  {
                     if( xmlSubReader.NodeType != XmlNodeType.Element ) continue;

                     if( xmlTextReader.Name == "BlockSize" )
                     {
                        XmlReader xmlSubSubReader = xmlTextReader.ReadSubtree( );
                        while( xmlSubSubReader.Read( ) )
                        {
                           if( xmlSubSubReader.NodeType != XmlNodeType.Element ) continue;

                           if( xmlTextReader.Name == "Heightmap" )
                              comboBoxHeightmapBlockSize.Text = xmlTextReader.ReadElementString( "Heightmap" );

                           if( xmlTextReader.Name == "Normalmap" )
                              comboBoxNormalmapBlockSize.Text = xmlTextReader.ReadElementString( "Normalmap" );

                           if( xmlTextReader.Name == "Overlaymap" )
                              comboBoxOverlaymapBlockSize.Text = xmlTextReader.ReadElementString( "Overlaymap" );

                        }
                        xmlSubSubReader.Close( );
                     }

                     if( xmlTextReader.Name == "LODoffset" )
                     {
                        XmlReader xmlSubSubReader = xmlTextReader.ReadSubtree( );
                        while( xmlSubSubReader.Read( ) )
                        {
                           if( xmlSubSubReader.NodeType != XmlNodeType.Element ) continue;

                           if( xmlTextReader.Name == "Heightmap" )
                              numericUpDownHeightmapLODoffset.Value = xmlTextReader.ReadElementContentAsDecimal( );

                           if( xmlTextReader.Name == "Normalmap" )
                              numericUpDownNormalmapLODoffset.Value = xmlTextReader.ReadElementContentAsDecimal( );

                           if( xmlTextReader.Name == "Overlaymap" )
                              numericUpDownOverlaymapLODoffset.Value = xmlTextReader.ReadElementContentAsDecimal( );

                        }
                        xmlSubSubReader.Close( );
                     }

                  }
                  xmlSubReader.Close( );
               }

               // read and set values for DataComplesion group
               if( xmlTextReader.Name == "DataComplesion" )
               {
                  XmlReader xmlSubReader = xmlTextReader.ReadSubtree( );
                  while( xmlSubReader.Read( ) )
                  {
                     if( xmlSubReader.NodeType != XmlNodeType.Element ) continue;

                     string is_checked;
                     if( xmlTextReader.Name == "HeightmapDXT5Compression" )
                     {
                        is_checked = xmlTextReader.ReadElementString( "HeightmapDXT5Compression" );
                        checkBoxHeightmapDXT5Compression.Checked = ( is_checked == "True" ) ? true : false;
                     }

                     if( xmlTextReader.Name == "OverlayImgCompression" )
                     {
                        is_checked = xmlTextReader.ReadElementString( "OverlayImgCompression" );
                        checkBoxOverlayImgCompression.Checked = ( is_checked == "True" ) ? true : false;
                     }

                     if( xmlTextReader.Name == "UseZLIBCompressio" )
                     {
                        is_checked = xmlTextReader.ReadElementString( "UseZLIBCompressio" );
                        checkBoxUseZLIBCompression.Checked = ( is_checked == "True" ) ? true : false;
                     }
                  }
                  xmlSubReader.Close( );
               }

               // read and set values for Rendering group
               if( xmlTextReader.Name == "Rendering" )
               {
                  XmlReader xmlSubReader = xmlTextReader.ReadSubtree( );
                  while( xmlSubReader.Read( ) )
                  {
                     if( xmlSubReader.NodeType != XmlNodeType.Element ) continue;

                     if( xmlTextReader.Name == "MinViewRange" )
                        numericUpDownMinViewRange.Value = xmlTextReader.ReadElementContentAsDecimal( );

                     if( xmlTextReader.Name == "MaxViewRange" )
                        numericUpDownMaxViewRange.Value = xmlTextReader.ReadElementContentAsDecimal( );

                     if( xmlTextReader.Name == "LODLevelDistanceRatio" )
                        numericUpDownLODLevelDistanceRatio.Value = xmlTextReader.ReadElementContentAsDecimal( );
                  }
                  xmlSubReader.Close( );
               }

               // read and set values for Rendering group
               if( xmlTextReader.Name == "DetailMap" )
               {
                  XmlReader xmlSubReader = xmlTextReader.ReadSubtree( );
                  while( xmlSubReader.Read( ) )
                  {
                     if( xmlSubReader.NodeType != XmlNodeType.Element ) continue;

                     if( xmlTextReader.Name == "X" )
                        numericUpDownDetailMapSizeX.Value = xmlTextReader.ReadElementContentAsDecimal( );

                     if( xmlTextReader.Name == "Y" )
                        numericUpDownDetailMapSizeY.Value = xmlTextReader.ReadElementContentAsDecimal( );

                     if( xmlTextReader.Name == "Z" )
                        numericUpDownDetailMapSizeZ.Value = xmlTextReader.ReadElementContentAsDecimal( );

                     if( xmlTextReader.Name == "DetailMapLODLevelsAffected" )
                        numericUpDownDetailMapLODLevelsAffected.Value = xmlTextReader.ReadElementContentAsDecimal( );

                     string is_checked;
                     if( xmlTextReader.Name == "DetailMapEnabled" )
                     {
                        is_checked = xmlTextReader.ReadElementString( "DetailMapEnabled" );
                        checkBoxDetailMapEnabled.Checked = ( is_checked == "True" ) ? true : false;
                     }
                  }
                  xmlSubReader.Close( );
               }
            }
            // reading of the file done
            xmlTextReader.Close( );
         }
      }

      private void buttonGo_Click( object sender, EventArgs e )
      {
         bool ok = ValidateParameters( );
         if( !ok ) return;

         string tempDir = "DLODWizardTemp_" + Guid.NewGuid( ).ToString( ) + @"\";
         tempDir = System.IO.Path.GetTempPath( ) + tempDir;

         Directory.CreateDirectory( tempDir );

         string iniFilePath = tempDir + "input.ini";

         // generate normal map in tempDir
         string normalMap = tempDir + "normalmap.tbmp";
         RunDemoExe( "-n " + "\"" + textBoxHeightmap.Text.ToString( ) + "\" " +
                     "\"" + normalMap + "\" " + numericUpDownWorldSizeX.Value.ToString( ) +
                     " " + numericUpDownWorldSizeY.Value.ToString( ) + " " +
                     numericUpDownWorldSizeZ.Value.ToString( ) );

         CreateIniFile( iniFilePath, normalMap );

         // call program
         RunDemoExe( "-c " + "\"" + iniFilePath + "\" " + "\"" + textBoxOutputSDLOD.Text + "\"" );
         // and than delete temorary folder
         Directory.Delete( tempDir, true );
      }

      private bool ValidateParameters( )
      {
         bool mapOk = ValidateMapParameters( );
         if( !mapOk ) return false;

         if( textBoxOutputSDLOD.Text == "" )
         {
            MessageBox.Show( "Output sdlod file must be chosen." );
            return false;
         }

         return true;
      }

      private bool ValidateMapParameters( )
      {
         if( textBoxHeightmap.Text == "" )
         {
            MessageBox.Show( "Heightmap must be selected." );
            return false;
         }
         if( textBoxOverlayImage.Text == "" ) return true;

         using( TiledBitmap heightmap = TiledBitmap.Open( textBoxHeightmap.Text, true ),
                  overlaymap = TiledBitmap.Open( textBoxOverlayImage.Text, true ) )
         {
            if( heightmap.PixelFormat != TiledBitmapPixelFormat.Format16BitGrayScale )
            {
               MessageBox.Show( "Heightmap format is not 16 bit grayscale." );
               return false;
            }
            if( overlaymap.PixelFormat != TiledBitmapPixelFormat.Format24BitRGB )
            {
               MessageBox.Show( "Overlay image format is not 24 bit RGB." );
               return false;
            }

            // check resolution
            if( overlaymap.Height % ( heightmap.Height - 1 ) != 0 ||
                overlaymap.Width % ( heightmap.Width - 1 ) != 0 )
            {
               MessageBox.Show( "Heightmap/overlaymap dimension mismatch.\n\n Overlaymap dimensions must be:\n" +
               " overlaymap.Height = n * (heightmap.Height - 1)\n overlaymap.Width  = n * (heightmap.Width - 1)\n" +
               " where n is an integer >= 1." );
               return false;
            }
         }

         return true;
      }

      private void CreateIniFile( string iniFilePath, string normalMap )
      {
         using( Stream file = File.Create( iniFilePath ) )
         {
            StreamWriter sw = new StreamWriter( file, Encoding.ASCII );

            sw.WriteLine( "[Main]" );
            sw.WriteLine( "" );
            sw.WriteLine( "# Size of the world" );
            sw.WriteLine( "MapDims_MinX = {0}", -(float)numericUpDownWorldSizeX.Value / 2.0f );
            sw.WriteLine( "MapDims_MinY = {0}", -(float)numericUpDownWorldSizeY.Value / 2.0f );
            sw.WriteLine( "MapDims_MinZ = {0}", 0.0 );
            sw.WriteLine( "MapDims_SizeX = {0}", numericUpDownWorldSizeX.Value );
            sw.WriteLine( "MapDims_SizeY = {0}", numericUpDownWorldSizeY.Value );
            sw.WriteLine( "MapDims_SizeZ = {0}", numericUpDownWorldSizeZ.Value );

            sw.WriteLine( "" );
            sw.WriteLine( "[SourceData]" );
            sw.WriteLine( "HeightmapPath = " + "\"" + textBoxHeightmap.Text.ToString( ) + "\"" );
            sw.WriteLine( "NormalmapPath = " + "\"" + normalMap + "\"" );
            sw.WriteLine( "OverlaymapPath = " + "\"" + textBoxOverlayImage.Text.ToString( ) + "\"" );
            sw.WriteLine( "HeightmapBlockSize = " + comboBoxHeightmapBlockSize.Text.ToString( ) );
            sw.WriteLine( "NormalmapBlockSize = " + comboBoxNormalmapBlockSize.Text.ToString( ) );
            sw.WriteLine( "OverlaymapBlockSize = " + comboBoxOverlaymapBlockSize.Text.ToString( ) );
            sw.WriteLine( "HeightmapLODOffset = {0}", numericUpDownHeightmapLODoffset.Value );
            sw.WriteLine( "NormalmapLODOffset = {0}", numericUpDownNormalmapLODoffset.Value );
            sw.WriteLine( "OverlaymapLODOffset = {0}", numericUpDownOverlaymapLODoffset.Value );
            sw.WriteLine( "NormalmapUseDXT5NMCompression = {0}", ( checkBoxHeightmapDXT5Compression.Checked == true ) ? 1 : 0 );
            sw.WriteLine( "OverlaymapUseDXT1Compression = {0}", ( checkBoxOverlayImgCompression.Checked == true ) ? 1 : 0 );
            sw.WriteLine( "UseZLIBCompression = {0}", ( checkBoxUseZLIBCompression.Checked == true ) ? 1 : 0 );

            sw.WriteLine( "" );
            sw.WriteLine( "[DLOD]" );
            sw.WriteLine( "LeafQuadTreeNodeSize = " + comboBoxLeafQuadtreeNodeSize.Text.ToString( ) );
            sw.WriteLine( "RenderGridResolutionMult = " + comboBoxRenderGridResolutionMultiplier.Text.ToString( ) );
            sw.WriteLine( "LODLevelCount = {0}", numericUpDownLODLevelCount.Value );
            sw.WriteLine( "LODLevelDistanceRatio = {0}", numericUpDownLODLevelDistanceRatio.Value );

            sw.WriteLine( "" );
            sw.WriteLine( "[Rendering]" );
            sw.WriteLine( "MinViewRange = {0}", numericUpDownMinViewRange.Value );
            sw.WriteLine( "MaxViewRange = {0}", numericUpDownMaxViewRange.Value );

            sw.WriteLine( "" );
            sw.WriteLine( "[DetailHeightmap]" );
            sw.WriteLine( "Enabled = {0}", ( checkBoxDetailMapEnabled.Checked == true ) ? 1 : 0 );
            sw.WriteLine( "SizeX = {0}", numericUpDownDetailMapSizeX.Value );
            sw.WriteLine( "SizeY = {0}", numericUpDownDetailMapSizeY.Value );
            sw.WriteLine( "SizeZ = {0}", numericUpDownDetailMapSizeZ.Value );
            sw.WriteLine( "MeshLODLevelsAffected = {0}", numericUpDownDetailMapLODLevelsAffected.Value );

            sw.Close( );
         }
      }

      private void RunDemoExe( string parameters )
      {
         // find path to the executable file
         string exeFile = System.IO.Path.GetDirectoryName( Application.ExecutablePath );
         exeFile += "\\DLOD_01_Streaming.exe";

         System.Diagnostics.ProcessStartInfo pci = new System.Diagnostics.ProcessStartInfo( exeFile, parameters );
         pci.UseShellExecute = false;

         System.Diagnostics.Process process = System.Diagnostics.Process.Start( pci );
         process.WaitForExit( );
      }

      // test function
      private void RunCmd( string arguments )
      {
         arguments = "/C \"" + arguments + "\"";
         System.Diagnostics.ProcessStartInfo pci = new System.Diagnostics.ProcessStartInfo( "cmd.exe", arguments );
         pci.RedirectStandardOutput = true;
         pci.RedirectStandardError = true;
         pci.CreateNoWindow = true;
         pci.UseShellExecute = false;

         System.Diagnostics.Process process = System.Diagnostics.Process.Start( pci );
         process.WaitForExit( );
         AppendOutputText( process.StandardOutput.ReadToEnd( ) + process.StandardError.ReadToEnd( ) );
      }

      // test function
      private void AppendOutputText( string text )
      {
         //richTextBoxStats.AppendText(text);
         //richTextBoxStats.SelectionStart = richTextBoxStats.Text.Length - 1;
         //richTextBoxStats.SelectionLength = 0;
         //richTextBoxStats.ScrollToCaret( );
      }


      private void buttonClearOverlaymapPath_Click( object sender, EventArgs e )
      {
         textBoxOverlayImage.Text = "";
      }


   }
}