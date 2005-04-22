/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkITKArchetypeImageSeriesReader.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkITKArchetypeImageSeriesReader.h"

#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

#ifndef vtkFloatingPointType
#define vtkFloatingPointType float
#endif

#include "itkExceptionObject.h"

// turn itk exceptions into vtk errors
#undef itkExceptionMacro  
#define itkExceptionMacro(x) \
  { \
  ::itk::OStringStream message; \
  message << "itk::ERROR: " << this->GetNameOfClass() \
          << "(" << this << "): " x; \
  std::cerr << message.str().c_str() << std::endl; \
  }

#undef itkGenericExceptionMacro  
#define itkGenericExceptionMacro(x) \
  { \
  ::itk::OStringStream message; \
  message << "itk::ERROR: " x; \
  std::cerr << message.str().c_str() << std::endl; \
  }

#include "itkArchetypeSeriesFileNames.h"
#include "itkImage.h"
#include "itkOrientImageFilter.h"
#include "itkImageSeriesReader.h"
#include "itkImageFileReader.h"
#include "itkImportImageContainer.h"
#include "itkImageRegion.h"
#include "itkGDCMSeriesFileNames.h"
#include "itkGDCMImageIO.h"
#include <itksys/SystemTools.hxx>

vtkCxxRevisionMacro(vtkITKArchetypeImageSeriesReader, "$Revision: 1.7 $");
vtkStandardNewMacro(vtkITKArchetypeImageSeriesReader);

//----------------------------------------------------------------------------
vtkITKArchetypeImageSeriesReader::vtkITKArchetypeImageSeriesReader()
{
  this->Archetype = NULL;
  this->SetDesiredCoordinateOrientationToAxial();
  this->FileNameSliceOffset = 0;
  this->FileNameSliceSpacing = 1;
  this->FileNameSliceCount = 0;

  this->OutputScalarType = VTK_FLOAT;
  for (int i = 0; i < 3; i++)
    {
    this->DefaultDataSpacing[i] = 1.0;
    this->DefaultDataOrigin[i] = 0.0;
    }
}

//----------------------------------------------------------------------------
vtkITKArchetypeImageSeriesReader::~vtkITKArchetypeImageSeriesReader()
{ 
  if (this->Archetype)
    {
    delete [] this->Archetype;
    this->Archetype = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkITKArchetypeImageSeriesReader::PrintSelf(ostream& os, vtkIndent indent)
{
  int idx;
  
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Archetype: " <<
    (this->Archetype ? this->Archetype : "(none)") << "\n";

  os << indent << "FileNameSliceOffset: " 
     << this->FileNameSliceOffset << "\n";
  os << indent << "FileNameSliceSpacing: " 
     << this->FileNameSliceSpacing << "\n";
  os << indent << "FileNameSliceCount: " 
     << this->FileNameSliceCount << "\n";

  os << indent << "OutputScalarType: "
     << vtkImageScalarTypeNameMacro(this->OutputScalarType)
     << std::endl;
  os << indent << "DefaultDataSpacing: (" << this->DefaultDataSpacing[0];
  for (idx = 1; idx < 3; ++idx)
    {
    os << ", " << this->DefaultDataSpacing[idx];
    }
  os << ")\n";
  
  os << indent << "DefaultDataOrigin: (" << this->DefaultDataOrigin[0];
  for (idx = 1; idx < 3; ++idx)
    {
    os << ", " << this->DefaultDataOrigin[idx];
    }
  os << ")\n";
  
}


//----------------------------------------------------------------------------
// This method returns the largest data that can be generated.
void vtkITKArchetypeImageSeriesReader::ExecuteInformation()
{
  vtkImageData *output = this->GetOutput();
  std::vector<std::string> candidateFiles;
  int extent[6];  

  // Test whether the input file is a DICOM file
  itk::GDCMImageIO::Pointer dicomIO = itk::GDCMImageIO::New();
  bool isDicomFile = dicomIO->CanReadFile(this->Archetype);
  if (isDicomFile)
    {
    typedef itk::GDCMSeriesFileNames DICOMNameGeneratorType;
    DICOMNameGeneratorType::Pointer inputImageFileGenerator = DICOMNameGeneratorType::New();  
    std::string fileNameName = itksys::SystemTools::GetFilenameName( this->Archetype );
    std::string fileNamePath = itksys::SystemTools::GetFilenamePath( this->Archetype );
    if (fileNamePath == "")
      {
      fileNamePath = ".";
      }
    inputImageFileGenerator->SetInputDirectory( fileNamePath );
    candidateFiles = inputImageFileGenerator->GetInputFileNames();
    if (candidateFiles.size() == 0)
      {
      candidateFiles.push_back(this->Archetype);
      }
    }
  else
    {  
    // Generate filenames from the Archetype
    itk::ArchetypeSeriesFileNames::Pointer fit = itk::ArchetypeSeriesFileNames::New();
    fit->SetArchetype (this->Archetype);
    candidateFiles = fit->GetFileNames();
    }

  // Reduce the selection of filenames
  int lastFile;
  if (this->FileNameSliceCount == 0)
    {
    lastFile = candidateFiles.size();
    }
  else
    {
    lastFile = this->FileNameSliceCount;
    }

  this->FileNames.resize(0);
  for (int f = this->FileNameSliceOffset;
       f < lastFile;
       f += this->FileNameSliceSpacing)
    {
    this->FileNames.push_back(candidateFiles[f]);
    }

  vtkFloatingPointType spacing[3];
  vtkFloatingPointType origin[3];
  
  // Since we only need origin, spacing and extents, we can use one
  // image type.
  typedef itk::Image<float,3> ImageType;
  itk::ImageRegion<3> region;

  // If there is only one file in the series, just use an image file reader
  if (this->FileNames.size() == 1)
    {
    itk::ImageFileReader<ImageType>::Pointer imageReader =
      itk::ImageFileReader<ImageType>::New();
    imageReader->SetImageIO(dicomIO);
    imageReader->SetFileName(this->FileNames[0].c_str());
    imageReader->GenerateOutputInformation();
    for (int i = 0; i < 3; i++)
      {
      spacing[i] = imageReader->GetOutput()->GetSpacing()[i];
      origin[i] = imageReader->GetOutput()->GetOrigin()[i];
      }
    region = imageReader->GetOutput()->GetLargestPossibleRegion();
    extent[0] = region.GetIndex()[0];
    extent[1] = region.GetIndex()[0] + region.GetSize()[0] - 1;
    extent[2] = region.GetIndex()[1];
    extent[3] = region.GetIndex()[1] + region.GetSize()[1] - 1;
    extent[4] = region.GetIndex()[2];
    extent[5] = region.GetIndex()[2] + region.GetSize()[2] - 1;
    }
  else
    {
    itk::ImageSeriesReader<ImageType>::Pointer seriesReader =
      itk::ImageSeriesReader<ImageType>::New();
    seriesReader->SetImageIO(dicomIO);
    seriesReader->SetFileNames(this->FileNames);
    
    itk::OrientImageFilter<ImageType,ImageType>::Pointer orient =
      itk::OrientImageFilter<ImageType,ImageType>::New();
    orient->SetInput(seriesReader->GetOutput());
    orient->UseImageDirectionOn();
    orient->SetDesiredCoordinateOrientation(this->DesiredCoordinateOrientation);
    orient->UpdateOutputInformation();

    for (int i = 0; i < 3; i++)
      {
      spacing[i] = orient->GetOutput()->GetSpacing()[i];
      origin[i] = orient->GetOutput()->GetOrigin()[i];
      }

    region = orient->GetOutput()->GetLargestPossibleRegion();
    extent[0] = region.GetIndex()[0];
    extent[1] = region.GetIndex()[0] + region.GetSize()[0] - 1;
    extent[2] = region.GetIndex()[1];
    extent[3] = region.GetIndex()[1] + region.GetSize()[1] - 1;
    extent[4] = region.GetIndex()[2];
    extent[5] = region.GetIndex()[2] + region.GetSize()[2] - 1;
    }

  // If it looks like the pipeline did not provide the spacing and
  // origin, modify the spacing and origin with the defaults
  for (int j = 0; j < 3; j++)
    {
    if (spacing[j] == 1.0)
      {
      spacing[j] = this->DefaultDataSpacing[j];
      }
    if (origin[j] == 0.0)
      {
      origin[j] = this->DefaultDataOrigin[j];
      }
    }

  output->SetSpacing(spacing);
  output->SetOrigin(origin);

  output->SetWholeExtent(extent);

  output->SetScalarType(this->OutputScalarType);
  output->SetNumberOfScalarComponents(1);
}

//----------------------------------------------------------------------------
// This function reads a data from a file.  The datas extent/axes
// are assumed to be the same as the file extent/order.
void vtkITKArchetypeImageSeriesReader::ExecuteData(vtkDataObject *output)
{
  itk::GDCMImageIO::Pointer dicomIO = itk::GDCMImageIO::New();

  if (!this->Archetype)
    {
    vtkErrorMacro("An Archetype must be specified.");
    return;
    }

  vtkImageData *data = vtkImageData::SafeDownCast(output);
  data->UpdateInformation();
  data->SetExtent(0,0,0,0,0,0);
  data->AllocateScalars();
  data->SetExtent(data->GetWholeExtent());
#define vtkITKExecuteDataFromSeries(typeN, type) \
    case typeN: \
    {\
      typedef itk::Image<type,3> image##typeN;\
      itk::ImageSeriesReader<image##typeN>::Pointer reader##typeN = \
            itk::ImageSeriesReader<image##typeN>::New(); \
      reader##typeN->SetFileNames(this->FileNames); \
      reader##typeN->SetImageIO(dicomIO); \
      reader##typeN->ReleaseDataFlagOn(); \
      itk::OrientImageFilter<image##typeN,image##typeN>::Pointer orient##typeN = \
            itk::OrientImageFilter<image##typeN,image##typeN>::New(); \
      orient##typeN->SetInput(reader##typeN->GetOutput()); \
      orient##typeN->UseImageDirectionOn(); \
      orient##typeN->SetDesiredCoordinateOrientation(this->DesiredCoordinateOrientation); \
      orient##typeN->UpdateLargestPossibleRegion();\
      itk::ImportImageContainer<unsigned long, type>::Pointer PixelContainer##typeN;\
      PixelContainer##typeN = orient##typeN->GetOutput()->GetPixelContainer();\
      void *ptr = static_cast<void *> (PixelContainer##typeN->GetBufferPointer());\
      (dynamic_cast<vtkImageData *>( output))->GetPointData()->GetScalars()->SetVoidArray(ptr, PixelContainer##typeN->Size(), 0);\
      PixelContainer##typeN->ContainerManageMemoryOff();\
    }\
    break

#define vtkITKExecuteDataFromFile(typeN, type) \
    case typeN: \
    {\
      typedef itk::Image<type,3> image2##typeN;\
      itk::ImageFileReader<image2##typeN>::Pointer reader2##typeN = \
            itk::ImageFileReader<image2##typeN>::New(); \
      reader2##typeN->SetFileName(this->FileNames[0].c_str()); \
      itk::OrientImageFilter<image2##typeN,image2##typeN>::Pointer orient2##typeN = \
            itk::OrientImageFilter<image2##typeN,image2##typeN>::New(); \
      orient2##typeN->SetInput(reader2##typeN->GetOutput()); \
      orient2##typeN->UseImageDirectionOn(); \
      orient2##typeN->SetDesiredCoordinateOrientation(this->DesiredCoordinateOrientation); \
      orient2##typeN->UpdateLargestPossibleRegion();\
      itk::ImportImageContainer<unsigned long, type>::Pointer PixelContainer2##typeN;\
      PixelContainer2##typeN = orient2##typeN->GetOutput()->GetPixelContainer();\
      void *ptr = static_cast<void *> (PixelContainer2##typeN->GetBufferPointer());\
      (dynamic_cast<vtkImageData *>( output))->GetPointData()->GetScalars()->SetVoidArray(ptr, PixelContainer2##typeN->Size(), 0);\
      PixelContainer2##typeN->ContainerManageMemoryOff();\
    }\
    break

  // If there is only one file in the series, just use an image file reader
  if (this->FileNames.size() == 1)
    {
    switch (this->OutputScalarType)
      {
      vtkITKExecuteDataFromFile(VTK_DOUBLE, double);
      vtkITKExecuteDataFromFile(VTK_FLOAT, float);
      vtkITKExecuteDataFromFile(VTK_LONG, long);
      vtkITKExecuteDataFromFile(VTK_UNSIGNED_LONG, unsigned long);
      vtkITKExecuteDataFromFile(VTK_INT, int);
      vtkITKExecuteDataFromFile(VTK_UNSIGNED_INT, unsigned int);
      vtkITKExecuteDataFromFile(VTK_SHORT, short);
      vtkITKExecuteDataFromFile(VTK_UNSIGNED_SHORT, unsigned short);
      vtkITKExecuteDataFromFile(VTK_CHAR, char);
      vtkITKExecuteDataFromFile(VTK_UNSIGNED_CHAR, unsigned char);
      default:
        vtkErrorMacro(<< "UpdateFromFile: Unknown data type");
      }
    }
  else
    {
    switch (this->OutputScalarType)
      {
      vtkITKExecuteDataFromSeries(VTK_DOUBLE, double);
      vtkITKExecuteDataFromSeries(VTK_FLOAT, float);
      vtkITKExecuteDataFromSeries(VTK_LONG, long);
      vtkITKExecuteDataFromSeries(VTK_UNSIGNED_LONG, unsigned long);
      vtkITKExecuteDataFromSeries(VTK_INT, int);
      vtkITKExecuteDataFromSeries(VTK_UNSIGNED_INT, unsigned int);
      vtkITKExecuteDataFromSeries(VTK_SHORT, short);
      vtkITKExecuteDataFromSeries(VTK_UNSIGNED_SHORT, unsigned short);
      vtkITKExecuteDataFromSeries(VTK_CHAR, char);
      vtkITKExecuteDataFromSeries(VTK_UNSIGNED_CHAR, unsigned char);
      default:
        vtkErrorMacro(<< "UpdateFromFile: Unknown data type");
      }
    }
}
