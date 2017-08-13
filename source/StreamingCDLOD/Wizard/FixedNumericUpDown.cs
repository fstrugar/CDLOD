using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;

namespace Wizard
{
   /// <summary>
   /// ReadOnly property of NumericUpDown does not prevent value change
   /// using up/down arrows, so we need to handle it like this
   /// </summary>
 
   public class FixedNumericUpDown : NumericUpDown
   {
      public override void DownButton( )
      {
         if( ReadOnly )
            return;
         base.DownButton( );
      }

      public override void UpButton( )
      {
         if( ReadOnly )
            return;
         base.UpButton( );
      }
   }
}
