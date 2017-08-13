using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.IO;

namespace ShaderReloader
{
	public partial class ShaderReloader// : Form
	{
		//private bool				active					= false;
		//private DateTime			fileLastWriteTime		= new DateTime(1903, 12, 17);
		//private LogForm				logForm					= null;
		//private int					logFormVisibleTickCount = 0;

		public ShaderReloader( )
		{
			//InitializeComponent( );
		}

		private void ShaderReloader_Load( object sender, EventArgs e )
		{
         /*
			OnActiveChanged( );

			Timer t = new Timer();
			
			t.Interval = 100;
			t.Tick += new EventHandler( Timer_Tick );
			t.Start();

			logForm = new LogForm();
			//logForm.Show();
			logForm.Visible = false;
			Screen screen = System.Windows.Forms.Screen.FromControl( this );
			Point logFormPos = new Point( screen.WorkingArea.Right - logForm.Size.Width, screen.WorkingArea.Top );
			logForm.Location = logFormPos;

			logForm.Initialise(this);

			checkBox1.Checked = true;
          * */
		}

		private void buttonPathBrowse_Click( object sender, EventArgs e )
		{
        
			OpenFileDialog dlg = new OpenFileDialog();
			dlg.InitialDirectory = @"C:\Code\ASURA_ROOT\Asura\Xbox360\Shaders\";
			dlg.Filter = "360 shader effect file (*.fx)|*.fx";

			if( dlg.ShowDialog() == DialogResult.OK)
			{
				//textBoxEffectPath.Text = dlg.FileName;
			}
       
		}

		private void textBoxEffectPath_TextChanged( object sender, EventArgs e )
		{
         /*
			if( File.Exists(textBoxEffectPath.Text) )
			{
				textBoxOutputPath.Text  = @"devkit:\_ShaderReloader\" + Path.GetFileNameWithoutExtension(textBoxEffectPath.Text) + ".bin";
				buttonStartStop.Enabled = true;
			}
			else
			{
				textBoxOutputPath.Text  = "";
				buttonStartStop.Enabled = false;
			}
          * */
		}

		void AppendLogText( string text )
		{
         //richTextBoxLog.AppendText(text);
         //richTextBoxLog.SelectionStart = richTextBoxLog.Text.Length - 1;
         //richTextBoxLog.SelectionLength = 0;
         //richTextBoxLog.ScrollToCaret( );
		}

		void RunCmd( string arguments )
		{
			arguments = "/C \"" + arguments + "\"";
			System.Diagnostics.ProcessStartInfo pci = new System.Diagnostics.ProcessStartInfo( "cmd.exe", arguments );
			pci.RedirectStandardOutput = true;
			pci.RedirectStandardError = true;
			pci.CreateNoWindow = true;
			pci.UseShellExecute = false;

			System.Diagnostics.Process process = System.Diagnostics.Process.Start( pci );
			process.WaitForExit( );
			AppendLogText( process.StandardOutput.ReadToEnd( ) + process.StandardError.ReadToEnd( ) );
		}

		void DoCompileAndCopy(string shaderPath, string destinationXbPath)
		{
			string shaderName = Path.GetFileNameWithoutExtension(shaderPath);

			AppendLogText( string.Format("\nExecuting build ({0})...\n\n", DateTime.Now.ToString()) );
			try
			{
				string tempPath = @"C:\Temp\";
				RunCmd( @"""C:\Program Files (x86)\Microsoft Xbox 360 SDK\Bin\Win32\fxc.exe"" /nologo /T fxl_3_0 /XfxPv /XZi " + 
						@"/XFd """ + tempPath + shaderName + @".updb"" " + 
						@"/Fo """ + tempPath + shaderName + @".bin"" " 
						+ shaderPath );
				
				// Copy to xbox
				RunCmd( @"""C:\Program Files (x86)\Microsoft Xbox 360 SDK\Bin\Win32\xbcp.exe"" /Y /T "
						+ "\"" + tempPath + shaderName + @".bin"" " + destinationXbPath );
				
				// Delete temp files
				RunCmd( @"del """ + tempPath + shaderName + @".bin""" );
			}
			catch (System.Exception ex)
			{
				AppendLogText( ex.Message );
			}
			AppendLogText( "\n---\n" );
         /*
			if( !this.ContainsFocus && checkBox1.Checked )
			{
				logForm.Visible = true;
				logFormVisibleTickCount = 50;
			}
          * */
		}

		void Timer_Tick( object sender, EventArgs e )
		{
         /*
			if( active )
			{
				try
				{
					string shaderPath = textBoxEffectPath.Text;
					DateTime fileWriteTime = File.GetLastWriteTime(shaderPath);

					if( fileWriteTime > fileLastWriteTime )
					{
						DoCompileAndCopy(shaderPath, textBoxOutputPath.Text);
						fileLastWriteTime = fileWriteTime;
					}
				}
				catch (System.Exception ex)
				{
					AppendLogText("\nError while updating: " + ex.Message + "\nStopping...\n");
					active = false;
					OnActiveChanged();
				}
				
			}

			if( logFormVisibleTickCount > 0 )
			{
				logFormVisibleTickCount--;
				if( logFormVisibleTickCount == 0 )
				{
					logForm.Visible = false;
				}
			}
         */

		}

		private string			cppCode = @"
//////////////////////////////////////////////////////////////////////////
static int g_uReloadShaderTickCount = 0;

static bool ReloadShader( Asura_Xbox360_Effect*& pxEffectRef )
{
	g_uReloadShaderTickCount++;
	if( g_uReloadShaderTickCount % 5 == 0 )
	{
		const char* szFileName = <shader_path>;

		HANDLE xFile = ::CreateFile( szFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
		if( xFile != INVALID_HANDLE_VALUE )
		{
			Asura_Xbox360_Effect* pxReloadedEff = NULL;
			if( g_uReloadShaderTickCount != 0 )				// ignore first file if it exists
			{
				u_int uSize = ::GetFileSize( xFile, NULL );
				char * pBuffer = new char[uSize];

				u_int uBytesRead = 0;
				::ReadFile( xFile, pBuffer, uSize, (LPDWORD)&uBytesRead, NULL );

				pxReloadedEff = Asura_Xbox360_Effect_System::CreateEffectFromMemory( pBuffer );
				delete[] pBuffer;
			}
			::CloseHandle( xFile );
			::DeleteFile( szFileName );

			if( pxReloadedEff != NULL )
			{
				if( pxEffectRef != NULL )
				{
					Asura_Xbox360_Effect_System::Destroy( pxEffectRef );
				}
				pxEffectRef = pxReloadedEff;
				return true;
			}
			return false;
		}
	}
	return false;
}
//////////////////////////////////////////////////////////////////////////
";

		private void button1_Click( object sender, EventArgs e )
		{
         /*
			StringBuilder path = new StringBuilder(textBoxOutputPath.Text);
			path.Replace("\\", "\\\\");
			StringBuilder code = new StringBuilder(cppCode);
			code.Replace("<shader_path>", "\""+path.ToString()+ "\"" );

			Form showCodeForm = new Form();
			showCodeForm.Size = new Size( 900, 600 );

			RichTextBox textBox = new RichTextBox();
			textBox.Text = code.ToString();
			textBox.ReadOnly = true;
			textBox.Dock = DockStyle.Fill;

			textBox.SelectAll();

			showCodeForm.Controls.Add(textBox);

			showCodeForm.ShowDialog();
          * */
		}

		private void notifyIcon1_MouseDoubleClick( object sender, MouseEventArgs e )
		{
         //Show( );
         //WindowState = FormWindowState.Normal;
		}

		private void ShaderReloader_Resize( object sender, EventArgs e )
		{
         //if( FormWindowState.Minimized == WindowState )
         //   Hide( );
		}

		private void closeToolStripMenuItem_Click( object sender, EventArgs e )
		{
			//this.Close();
		}

		private void notifyIconSysTray_MouseClick( object sender, MouseEventArgs e )
		{
			//Show( );
			//WindowState = FormWindowState.Normal;
		}

	}
}