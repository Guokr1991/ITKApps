/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    vhunchunker.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 2002 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#include <string>
#include <iostream>
#include "param.h"
#include "chunks.h"
extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
}

/*
  This application joins raw volume files from raw chunks.  The number,
  name, and
  size of the chunks are specified in the input parameter file, chunks.in (or
  filename supplied on the command line).  Parameter files look like the
  following with parameters described below

  This application can assemble the output into raw volumes, or it can output
  raw slices.  This application can be used to ``undo'' the chunking of a
  volume by either the vhchunker or chunker applications.  See also
  documentation for vhchunker.cxx and chunker.cxx.

  Padding is automatically removed during reassembly.
  
  The default input parameter file is unchunk.in.  unchunk.in must define the
  following parameters:

  (chunk_file       "/path/to/chunk/record/file/chunkset.record")
  (chunk_prefix     "/path/to/chunk/files/chunkfileprefix")
  (volume_extent     724 1224 308 908 0 854)       // Size of the whole volume
  (reassemble_extent 724 1024 408 808 100 200)     // Area to reassemble
  (pixel_components 3)   // Number of component values contained in each pixel
  (pixel_size 4)         // Size in bytes of each pixel component
  (pad 20 10 10)         // Size of the padding included in each chunk
                         //(the padding
                         // will be cropped out of the reassembled volume)

  If reassembling into a single, raw volume file, supply the following
  (volume_name      "/path/to/reassembled/volume/file/volume_name.raw")

  If reassembling into a set of slices, supply the following:
  (slice_prefix     "/path/to/slice/files/slicefileprefix")
  
  Requires the vispack ParameterFile library libparam

*/

class ra_file
{
public:
  ra_file() {}
  virtual void open(const char *fn) = 0;
  
  virtual void write(long x, long y, long z,
                     char *buf, long sz) = 0;
  virtual void close() { m_ostream.close(); }

  void size(const long x,
            const long y,
            const long z) { m_sz[0] = x; m_sz[1] = y; m_sz[2] = z; }
  
  const long *size() const { return m_sz; }
  long size(long a) const { return m_sz[a]; }
  long pixel_size() const { return m_pixel_size; }
  void pixel_size( const long a) { m_pixel_size = a; }
  long components() const { return m_components; }
  void components(const long a) { m_components = a; }
  void z_offset(const long a) { m_z_offset = a; }
  long z_offset() const { return m_z_offset; }
protected:
  static long bufsz;
  long m_sz[3];
  long m_pixel_size;
  long m_components;
  long m_z_offset;

  // strides are in bytes
  long m_x_stride;
  long m_y_stride;
  long m_z_stride;

  std::ofstream m_ostream;

  void calculate_strides();
  
};

void ra_file::calculate_strides()
{
  m_x_stride = m_pixel_size * m_components;
  m_y_stride = m_sz[0] * m_x_stride;
  m_z_stride = m_sz[1] * m_y_stride;
}

long ra_file::bufsz = 65536;

class volume_ra_file : public ra_file
{
public:
  volume_ra_file() {}
  virtual void write(long x, long y, long z,
                     char *buf, long sz);
  virtual void open(const char *fn);
};

void volume_ra_file::open(const char *fn)
{
  ra_file::calculate_strides();
  char *buf;

  long counter = 0;
  long filelen = m_sz[0] * m_sz[1] * m_sz[2]
    * m_pixel_size * m_components; 

  // Create and pad file to proper size
  m_ostream.open(fn, std::ios::trunc);
  if (!m_ostream)
    {
      std::cerr << "Cannot open file " << fn << std::endl;
      ::exit(1);
    }
  buf = new char[bufsz];
  ::memset((void *)buf, 0, bufsz);
  while (counter < (filelen - bufsz))
    {
      m_ostream.write(buf, bufsz);
      counter += bufsz;
    }
  m_ostream.write(buf, filelen - counter);
}

void volume_ra_file::write(long x, long y, long z,
                           char *buf, long sz)
{
  long offset = x*m_x_stride + y*m_y_stride + z*m_z_stride;
  m_ostream.seekp(offset, std::ios::beg);
  m_ostream.write(buf, sz);
}

class slice_ra_file : public ra_file
{
public:
  slice_ra_file() {m_closed = true;}
  virtual void write(long x, long y, long z,
                     char *buf, long sz);
  virtual void open(const char *fn);
protected:
  bool m_closed;
  long m_current_z;
  std::string m_prefix;
};

void slice_ra_file::write(long x, long y, long z,
                          char *buf, long sz)
{
  std::string name;
  char nmbuf[255];
  if (m_closed == true || m_current_z != z)
    {
      m_current_z = z;
      if (m_closed != true) m_ostream.close();
      ::sprintf(nmbuf,"%u", z + m_z_offset);
      name = m_prefix + "." +  std::string(nmbuf);
      //      m_ostream.open(name.c_str(), std::ios::nocreate);
      m_ostream.open(name.c_str());
     
      if (!m_ostream)
        {
          std::cerr << "File " << name
                    << " does not exist or cannot be opened." << std::endl;
          exit(1);
        }
      m_closed = false;
    }
  long offset = x*m_x_stride + y*m_y_stride;
  m_ostream.seekp(offset, std::ios::beg);
  m_ostream.write(buf, sz);
}

void slice_ra_file::open(const char *fn)
{
    ra_file::calculate_strides();
  m_prefix = fn;
  m_closed = true;
  long zLast  = m_z_offset + m_sz[2];
  long filelen;
  std::string name;
  char *buf;
  char nmbuf[255];
  long counter = 0;

  // Create and pad the appropriate slice files
  for (unsigned i = m_z_offset; i < zLast; ++i)
    {
      ::sprintf(nmbuf, "%u", i);
      name = std::string(fn) + "." + std::string(nmbuf);
      m_ostream.open(name.c_str(), std::ios::trunc);
      if (!m_ostream)
        {
          std::cerr << "Could not create file " << name << std::endl;
          ::exit(1);
        }

      filelen = m_sz[0] * m_sz[1] * m_pixel_size * m_components;
      
      buf = new char[bufsz];
      ::memset((void *)buf, 0, bufsz);
      counter = 0;
      while (counter < (filelen - bufsz))
        {
          m_ostream.write(buf, bufsz);
          counter += bufsz;
        }
      m_ostream.write(buf, filelen - counter);
      m_ostream.close();
    }
}
  
inline void die(const char *s)
{
  std::cerr << s << std::endl;
  exit(-1);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
  ctk::chunk_info_struct *chunk_list;
  std::string output_name, chunk_filename, chunk_prefix;
  long pad[3], o_Xextent[2], o_Yextent[2], o_Zextent[2],
    volume_extent[6], pixel_size, pixel_components, i;
  bool slices;
  int nooffset;
  ra_file *outstream;
  
  std::string fn;
  char *default_fn = "unchunk.in";
  if (argc>1) fn = argv[1];
  else fn = default_fn;
  
  VPF::ParameterFile pf;
  // Open and parse parameter file
  pf. Initialize(fn.c_str());
  if (VPF::set(chunk_filename, pf["chunk_file"][0])==VPF::INVALID)
    die ("Cannot set chunk_file parameter");
  if (VPF::set(chunk_prefix, pf["chunk_prefix"][0])==VPF::INVALID)
    die ("Cannot set chunk_prefix parameter");
  if (VPF::set(pixel_size, pf["pixel_size"][0]) == VPF::INVALID)
    die ("Cannot set pixel_size parameter");
  if (VPF::set(pixel_components, pf["pixel_components"][0]) == VPF::INVALID)
    die ("Cannot set pixel_components parameter");
  if (VPF::set(pad[0], pf["pad"][0]) == VPF::INVALID)
    die ("Cannot set pad parameter");
  if (VPF::set(pad[1], pf["pad"][1]) == VPF::INVALID)
    die ("Cannot set pad parameter");
  if (VPF::set(pad[2], pf["pad"][2]) == VPF::INVALID)
    die ("Cannot set pad parameter");
  for (i = 0; i < 6; ++i)
    {
      if (VPF::set(volume_extent[i], pf["volume_extent"][i]) == VPF::INVALID)
        die ("ERROR setting volume_extent parameter");
    }
  if (VPF::set(o_Xextent[0], pf["reassemble_extent"][0]) == VPF::INVALID)
    die ("ERror setting reassemble_extent parameter");
  if (VPF::set(o_Xextent[1], pf["reassemble_extent"][1]) == VPF::INVALID)
    die ("ERror setting reassemble_extent parameter");
  if (VPF::set(o_Yextent[0], pf["reassemble_extent"][2]) == VPF::INVALID)
    die ("ERror setting reassemble_extent parameter");
  if (VPF::set(o_Yextent[1], pf["reassemble_extent"][3]) == VPF::INVALID)
    die ("ERror setting reassemble_extent parameter");
  if (VPF::set(o_Zextent[0], pf["reassemble_extent"][4]) == VPF::INVALID)
    die ("ERror setting reassemble_extent parameter");
  if (VPF::set(o_Zextent[1], pf["reassemble_extent"][5]) == VPF::INVALID)
    die ("ERror setting reassemble_extent parameter");

  
  if (VPF::set(output_name, pf["volume_name"][0]) == VPF::INVALID)
    slices = true;
  
  if (slices == true)
    {
      if ( VPF::set(output_name, pf["slice_prefix"][0]) == VPF::INVALID)
        die ("Cannot set slice_prefix");
    }
  if (VPF::set(nooffset, pf["no_z_offset"][0]) ==VPF::INVALID)
    {
      nooffset = 0;
    }
  
  
  // Open the chunk record file.  This will provide all the information we need 
  // to reassemble the chunks.
  long chunknumber;
  std::ifstream in;
  in.open(chunk_filename.c_str());
  if (!in)
    {
      std::cerr << "Could not open chunk record file " << chunk_filename
                << std::endl;
      ::exit(1);
    }
  in.read((char *)&chunknumber, sizeof(int));
  chunk_list = new ctk::chunk_info_struct[chunknumber];
  in.read((char *)chunk_list, chunknumber *
          sizeof(ctk::chunk_info_struct));
  in.close();

  // Some properties of the output volume that we are going to create
  long o_size[3];
  long c_Xstride, c_Ystride, c_Zstride;
  long startX, startY, startZ, endX, endY, endZ, c_Yoffset,
    c_Zoffset, c_endX, c_endY, c_endZ, c_start;  
  o_size[0] = o_Xextent[1] - o_Xextent[0];
  o_size[1] = o_Yextent[1] - o_Yextent[0];
  o_size[2] = o_Zextent[1] - o_Zextent[0];
  
  // Create & pad the output file(s)
  if (slices == true)
    {
       outstream = new slice_ra_file();
    }
  else outstream = new volume_ra_file();
  outstream->size(o_size[0], o_size[1], o_size[2]);
  outstream->pixel_size(pixel_size);
  outstream->components(pixel_components);
  if (nooffset == 0) outstream->z_offset(o_Zextent[0]);
  else outstream->z_offset(0);
  outstream->open(output_name.c_str());

  // Step through the chunks and assemble the output file(s),
  // cropping out padded regions.
  std::string chunk_name;
  char strbuf[255];
  char *scanbuf;
  long scanlen;
  std::cout << "Reconstructing the region (" << o_Xextent[0] << "-"
            << o_Xextent[1] << ", " << o_Yextent[0] << "-" << o_Yextent[1]
            << ", " << o_Zextent[0] << "-" << o_Zextent[1] << ")" << std::endl;
  
  for (i = 0; i < chunknumber; ++i)
    {
      ::sprintf(strbuf, "%u", i);
      chunk_name = chunk_prefix + "." + strbuf;

      //
      std::cout << "--------------------------------------------" << std::endl;
      std::cout << "Assembling chunk " << i << "/" << chunknumber - 1
                << " " << chunk_name << std::endl;
      //
      
      // Some properties of the current chunk
      c_Xstride = pixel_size * pixel_components;
      c_Ystride = chunk_list[i].get_padded_sz()[0] * c_Xstride;
      c_Zstride = chunk_list[i].get_padded_sz()[1] * c_Ystride;
      c_endX = chunk_list[i].get_region_idx()[0]
             + chunk_list[i].get_region_sz()[0];
      c_endY = chunk_list[i].get_region_idx()[1]
             + chunk_list[i].get_region_sz()[1];
      c_endZ = chunk_list[i].get_region_idx()[2]
             + chunk_list[i].get_region_sz()[2];

      if (o_Xextent[0] < chunk_list[i].get_region_idx()[0])
        startX = chunk_list[i].get_region_idx()[0];
      else startX = o_Xextent[0];

      if (o_Yextent[0] < chunk_list[i].get_region_idx()[1])
        startY = chunk_list[i].get_region_idx()[1];
      else startY = o_Yextent[0];

      if (o_Zextent[0] < chunk_list[i].get_region_idx()[2])
        startZ = chunk_list[i].get_region_idx()[2];
      else startZ = o_Zextent[0];

      if (startX > c_endX) continue;  // No need to mess with this chunk
      if (startY > c_endY) continue;
      if (startZ > c_endZ) continue;
      
      if (o_Xextent[1] > c_endX) endX = c_endX;
      else endX = o_Xextent[1];
      
      if (o_Yextent[1] > c_endY) endY = c_endY;
      else endY = o_Yextent[1];

      if (o_Zextent[1] > c_endZ) endZ = c_endZ;
      else endZ = o_Zextent[1];

      if (endX < chunk_list[i].get_region_idx()[0]) continue;
      if (endY < chunk_list[i].get_region_idx()[1]) continue;
      if (endZ < chunk_list[i].get_region_idx()[2]) continue;

      // Wrap-around offsets (bytes)
      c_Yoffset = (chunk_list[i].get_padded_sz()[0] - (endX - startX))
        * c_Xstride;
      c_Zoffset = (chunk_list[i].get_padded_sz()[1] - (endY - startY))
        * c_Ystride;

      // Starting offset (bytes)
      c_start = (startX -chunk_list[i].get_padded_idx()[0]) * c_Xstride
        + (startY - chunk_list[i].get_padded_idx()[1]) * c_Ystride
        + (startZ - chunk_list[i].get_padded_idx()[2]) * c_Zstride;
      
      scanlen = (endX - startX) * pixel_size * pixel_components;
      scanbuf = new char[scanlen];
      
      //
      std::cout << chunk_list[i] << std::endl;
      std::cout << "Copying from chunk: " << startX << " - " << endX << ", " << 
        startY << " - " << endY << ", " << startZ << "-" << endZ <<std::endl;
      std::cout << "Scanline has length " << scanlen << " bytes." << std::endl;
      //

      // Copy & Write the appropriate scanlines
      std::ifstream chunkin;
      chunkin.open(chunk_name.c_str());
      if (!chunkin)
        {
          die ("Could not open chunk file" );
        }
      chunkin.seekg(c_start, std::ios::beg);
      
      long xPos, yPos, zPos;
      xPos = startX - o_Xextent[0];
      for (long zz = startZ; zz < endZ; zz++)
        {
          zPos = zz- o_Zextent[0];
          for (long yy = startY; yy < endY; yy++)
            {
              yPos = yy - o_Yextent[0];
              chunkin.read((char *)scanbuf, scanlen);
              outstream->write(xPos, yPos, zPos,  scanbuf, scanlen);
              chunkin.seekg(c_Yoffset, std::ios::cur);
            }
          chunkin.seekg(c_Zoffset, std::ios::cur);
        }
      delete [] scanbuf;
    }

  outstream->close();
  delete outstream;
  return 0;
}
