/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    ImageColorViewer.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 2002 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#include <iostream>

#include <itkImage.h>
#include <itkRGBPixel.h>
#include <itkImageFileReader.h>

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/fl_file_chooser.H>
#include <GLColorSliceView.h>
#include <GLSliceView.h>
#include <SliceView.h>

#include "ImageColorViewerGUI.h"


Fl_Window *form;

int usage(void)
   {
   std::cout << "ImageColorViewer" << std::endl;
   std::cout << std::endl;
   std::cout << "ImageColorViewer <Filename>" << std::endl;
   std::cout << std::endl;

   return 1;
   }

int main(int argc, char **argv)
  {
  char *fName;

  if(argc > 2)
    {
    return usage();
    }
   else
    if(argc == 1)
      {
      fName = fl_file_chooser("Pick a Image file", "*.mh*", ".");
      if(fName == NULL || strlen(fName)<1)
        {
        return 0;
        }
      }
    else
      if(argv[1][0] != '-')
        {
        fName = argv[argc-1];
        }
      else
        {
        return usage();
        }

  std::cout << "Loading File: " << fName << std::endl;
  typedef itk::Image< itk::RGBPixel<float>, 3 > ImageType;
  typedef itk::ImageFileReader< ImageType > VolumeReaderType;
  VolumeReaderType::Pointer reader = VolumeReaderType::New();

  reader->SetFileName(fName);

  ImageType::Pointer imP;
  imP = reader->GetOutput();

  try
    {
    reader->Update();
    }
  catch( ... )
    {
    std::cout << "Problems reading file format" << std::endl;
    return 1;
    }
  std::cout << "...Done Loading File" << std::endl;

  char mainName[255];
  sprintf(mainName, "metaView: %s", fName);

  std::cout << std::endl;
  std::cout << "For directions on interacting with the window," << std::endl;
  std::cout << "   type 'h' within the window" << std::endl;
  
  form = make_window();

  tkMain->label(mainName);
  
  tkWin->SetInputImage(imP);
  
  form->show();
  tkWin->show();
  tkWin->update();

  Fl::run();
  
  return 1;
  }
