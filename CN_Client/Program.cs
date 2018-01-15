using System;
using System.Collections.Generic;
using System.Windows.Forms;

/*
 * $Id: Program.cs,v 1.1 2006/10/24 19:32:59 mkalewski Exp $
 */

namespace DayTime
{
    static class Program
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new Form1());
        }
    }
}