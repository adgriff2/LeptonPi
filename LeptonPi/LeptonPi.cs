using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Net.NetworkInformation;
using System.Diagnostics;
using System.Threading;

namespace LeptonPi
{
    public partial class LeptonPi : Form
    {
        public Bitmap image;
        public LeptonPi()
        {
            InitializeComponent();
        }

        delegate void updateImageDel(Bitmap bmp);

        private void updateImage(Bitmap bmp)
        {
            image = new Bitmap(bmp);
            pictureBox1.BackgroundImage = image;
            pictureBox1.Update();
            pictureBox1.Refresh();
        }

        private void getImage()
        {
            uint width = 80, height = 60, bytes_per_pixel = 2, bytes_per_header = 4;
            TcpClient client = new TcpClient();
            client.Connect(textBox1.Text,Convert.ToInt32(textBox2.Text));
            Stream s = client.GetStream();
            BinaryReader r = new BinaryReader(s);
            uint VOSPI_PACKET_SIZE = width * bytes_per_pixel + bytes_per_header;
            byte[] data = new byte[10 * height * VOSPI_PACKET_SIZE];
            ushort[] grayscale = new ushort[height * width];
            //byte[] headers = new byte[bytes_per_header * height];
            uint i, j, max, min;
            int n = 1;
            Bitmap bmp = new Bitmap((int)width, (int)height);
            uint i_width, i_vospi;

            while (n > 0)
            {
                n = r.Read(data, 0, data.Length);
                if (n < height * VOSPI_PACKET_SIZE)
                {
                    continue;
                }
                max = 0;
                min = UInt32.MaxValue;
                for (i = 0; i < height; i++)
                {
                    //for (j = 0; j < bytes_per_header; j++)
                    //{
                    //    headers[i * bytes_per_header + j] = data[i * VOSPI_PACKET_SIZE + j];
                    //}
                    i_width = i * width;
                    i_vospi = i * VOSPI_PACKET_SIZE;
                    for (j = bytes_per_header; j < VOSPI_PACKET_SIZE; j += 2)
                    {
                        grayscale[i * width + (j - bytes_per_header) / 2] = (ushort)(data[i_vospi + j] << 8 | data[i_vospi + j + 1]);
                        if (grayscale[i_width + (j - bytes_per_header) / 2] > max)
                        {
                            max = grayscale[i_width + (j - bytes_per_header) / 2];
                        }
                        if (grayscale[i_width + (j - bytes_per_header) / 2] < min)
                        {
                            min = grayscale[i_width + (j - bytes_per_header) / 2];
                        }
                    }
                }
                decimal min_max_255 = (decimal)255 / (decimal)(max - min);
                for (i = 0; i < height; i++)
                {
                    i_width = i * width;
                    for (j = 0; j < width; j++)
                    {
                        bmp.SetPixel((int)j, (int)i, Color.FromArgb((int)Math.Round((grayscale[i_width + j] - min) * min_max_255), 0, 0, 0));
                    }
                }
                pictureBox1.Invoke(new updateImageDel(updateImage), bmp);
            }
            s.Close();
            client.Close();
        }

        private void button1_Click(object sender, EventArgs e)
        {
            Thread workerThread = new Thread(getImage);
            workerThread.Start();
        }
    }
}
