#include <opencv2/core/core.hpp>  
#include <opencv2/highgui/highgui.hpp>  
#include <iostream>
#include <fstream>
#include <memory>
#include <bitset>

using namespace std;
using namespace cv;

/* carrier image */
const string c_path = "...";
/* file to embed; should be read as binary */
const string s_path = "...";
/* result */
const string r_path = "...";
/* file with surprises */
const string e_path = "...";

int read_image(Mat& image, const string path)
{
    image = imread(path);
    if (image.empty()) 
    {
        cout << "Could not open or find the image" << endl;
        cin.get(); 
        return -1;
    }
    return 0;
}


inline char get_bit(char var, int n)
{
    auto txt = bitset<8>(var).to_string();
    cout << txt << endl;
    cout << n << ' '<< txt[n] << endl;
    return txt[n];
}

Mat encode(Mat carrier, ifstream& file)
{
    auto file_size = int32_t(file.tellg());
    auto memblock = unique_ptr<char[]>(new char[file_size]); // don't need to delete memory later
    file.seekg(0, ios::beg);
    file.read(memblock.get(), file_size);
    int channel = 2;

    /* free space */
    auto slots = vector<Vec3i>(carrier.cols * carrier.rows);
    auto slots_it = slots.begin();
    for (int i = 0; i < carrier.rows; ++i)
        for (int j = 0; j < carrier.cols; ++j)
            
                *(slots_it++) = Vec3i({i, j, channel});
    slots.erase(slots_it, slots.end());  // now slots.size() is a number of free slots

    int iter = 0;
    /* hide file size */
  
    for (int i = 0; i < 32; ++i)
    {
        Vec3i slot = slots[iter++];
        int row = slot[0];
        int col = slot[1];
       
        char bit = bitset<32>(file_size).to_string()[i];
     
        carrier.at<Vec3b>(row, col)[channel] = carrier.at<Vec3b>(row, col)[channel] & 0xFE | bit;
 
    }
    

    for (int i = 0; i < file_size; ++i)
    {
        auto var = char(memblock[i]);

        string bits;
        for (int j = 0; j < 8; ++j)
       
        {
            Vec3i slot = slots[iter++];
            int row = slot[0];
            int col = slot[1];
          
            char bit = bitset<8>(var).to_string()[j];
   
            carrier.at<Vec3b>(row, col)[channel] = carrier.at<Vec3b>(row, col)[channel] & 0xFE | bit;
            
        }
        
    }

    return carrier;
}

// Mat encode(Mat carrier, ifstream& file)
// {
//     /* MSB of the secret file written into LSB of the carrier */
//     /* for each colour channel */
//     cout << "here"<<endl;

//     auto file_size = int32_t(file.tellg());
//     auto memblock = unique_ptr<char[]>(new char[file_size]); // don't need to delete memory later
//     file.seekg(0, ios::beg);
//     file.read(memblock.get(), file_size);
//     cout << "file"<<endl;
//     int channel = 2;

//     /* hide file size */
//     for (int i = 0; i<32; i++)
//     {
//         Vec3b pixel = carrier.at<Vec3b>(0, i);
//         pixel.val[channel] |= get_bit(file_size, i);
//     }
//    cout << "size "<< file_size << endl;

//     /* hide the file */
//     int i = 0;
//     unsigned int j = 0;
//     while (i < file_size) 
//     {
//         for (int row = 1; row < carrier.rows; row++)
//         {
//             for (int col = 0; col < carrier.cols; col++)
//             {
//                 //for (int channel = 0; channel < 3; channel++)
//                 //{
//                 // channel = 2;
//                 //Vec3b pixel = carrier.at<Vec3b>(row, col);
//                 /* LSB of cpixel = bit of the file */ 
//                 auto var = char(memblock[i]);
//                 int n = j;
//                 bool getbit = 1 & (((char*)&var)[sizeof(char) - n / 8 - 1] >> (n % 8));
//                 carrier.at<Vec3b>(row, col).val[channel] |= getbit;
//                 //cout << "i " << i << " j " << j << endl;
//                 /* update numbering */
//                 if (j==7) {i++; j = 0;}
//                 else {j++;}

//                 if (i >= file_size) {break;}
                
//                 /* update the image */
//                 //carrier.at<Vec3b>(row, col) = pixel;
//                 // WHY
//                 if (row==carrier.rows-1 and col==carrier.cols-1) {break;}
//                 if (i==file_size) {break;}
//                 //}
//             }
//             if (i >= file_size) {break;}

//         }
//         if (i >= file_size) {break;}
//     }
//     return carrier;
// }

Mat encode(const string c_path, const string s_path)
{
    Mat im;
    read_image(im, c_path);
    cout << im.rows << " " << im.cols << endl;
    /* read file to memory */
    ifstream file = ifstream(s_path, ios::binary | ios::ate | ios::in);
    
    return encode(im, file);
}


int decode (const string path)
{
    /* read image */
    Mat carrier;
    read_image(carrier, path);
    /* slots with data */
    /* free space */
    int channel = 2;
    auto slots = vector<Vec3i>(carrier.cols * carrier.rows);
    auto slots_it = slots.begin();
    for (int i = 0; i < carrier.rows; ++i)
        for (int j = 0; j < carrier.cols; ++j)
           
                *(slots_it++) = Vec3i({i, j, channel});
    slots.erase(slots_it, slots.end());  // now slots.size() is a number of free slots


    /* create file for output */
    /* size */
    int iter = 0;
    int32_t file_size = 0;
    bitset<32> fsize;
    for (int i = 31; i >= 0; --i)
    {
        Vec3i slot = slots[iter++];
        int row = slot[0];
        int col = slot[1];
        auto bit = bitset<8>(carrier.at<Vec3b>(row, col)[channel]).to_string()[7];
        if (bit=='1') {
            fsize.set(i);
        }
    }
    file_size = static_cast<int32_t>(fsize.to_ulong());
    cout << "file size " << file_size << endl;

    /* lock memory for file */
    auto memblock = unique_ptr<char[]>(new char[file_size]);
    for (int i = 0; i < file_size; ++i) 
    {
        bitset<8> bits;
        for (int j = 7; j >=0; j--) 
        {
            Vec3i slot = slots[iter++];
            int row = slot[0];
            int col = slot[1];
            auto byte = carrier.at<Vec3b>(row, col)[channel];
          
            /* need only the last bit */
            auto bit = bitset<8>(byte).to_string()[7];
            if (bit=='1') 
            {
                bits.set(j);
            }
        }

        unsigned long var = bits.to_ulong(); 
        memblock[i] = static_cast<unsigned char>(var);

    }

    string res_path = "res";
    ofstream file = ofstream(res_path, ios::out);
    if (!file.is_open()) {
        cout << "Could not open or find " << res_path << endl;
        return -1;
    }
    file.write(memblock.get(), file_size);
    return 0;
}

// Mat decode(Mat carrier)
// {
//     /* MSB of the secret image written into LSB of the carrier */
//     /* for each colour channel */
//     // make noise image of the correct size
//     // each (row, col) = LSB of the carrier (but make it MSB)
//     Mat res(Size(carrier.rows, carrier.cols), CV_64FC1);
//     for (int row = 0; row < carrier.rows; row++)
//     {
//         for (int col = 0; col < carrier.cols; col++)
//         {
//             for (int colour = 0; colour < 3; colour++)
//             {
//                 Vec3b carbit = carrier.at<Vec3b>(row, col);
//                 Vec3b secbit;
//                 auto bits = get_bits(carbit.val[colour]);
//                 /* BGR colour ordering */
//                 secbit.val[colour] = bits*128;
//             /* update the result image */
//             res.at<Vec3b>(row, col) = secbit;
//             }
//         }
//     }
//     // make res from all
//     return res;
// }

int main(){

    Mat res = encode(c_path, s_path);
    String windowName = "res"; 
    imwrite(r_path, res);
    int rs = decode(r_path);
    return rs;
  
}
