/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    IRISSlicer.txx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
template<class TPixel> 
IRISSlicer<TPixel>
::IRISSlicer()
{
  // There is a single input to the filter
  SetNumberOfRequiredInputs(1);

  // There are three outputs from the filter
  SetNumberOfRequiredOutputs(1);

  // Initialize the slice to be along the z-direction in the image
  m_SliceDirectionImageAxis = 2;
  m_LineDirectionImageAxis = 1;
  m_PixelDirectionImageAxis = 0;
  m_LineTraverseForward = true;
  m_LineTraverseForward = true;

  // Initialize to a zero slice index
  m_SliceIndex = 0;
}

template<class TPixel> 
void IRISSlicer<TPixel>
::GenerateOutputInformation()
{
  // Get pointers to the inputs and outputs
  typename Superclass::InputImageConstPointer inputPtr = GetInput();
  typename Superclass::OutputImagePointer outputPtr = GetOutput();
  
  // The inputs and outputs should exist
  if (!outputPtr || !inputPtr) return;

  // Get the input's largest possible region
  InputImageRegionType inputRegion = inputPtr->GetLargestPossibleRegion();
  
  // Arrays to specify the output spacing and origin
  double outputSpacing[2];
  double outputOrigin[2] = {0.0,0.0};
  
  // Initialize the output image region
  OutputImageRegionType outputRegion; 
  outputRegion.SetIndex(0,inputRegion.GetIndex(m_PixelDirectionImageAxis));
  outputRegion.SetSize(0,inputRegion.GetSize(m_PixelDirectionImageAxis));
  outputRegion.SetIndex(1,inputRegion.GetIndex(m_LineDirectionImageAxis));
  outputRegion.SetSize(1,inputRegion.GetSize(m_LineDirectionImageAxis));

  // Set the origin and spacing
  outputSpacing[0] = inputPtr->GetSpacing()[m_PixelDirectionImageAxis];
  outputSpacing[1] = inputPtr->GetSpacing()[m_LineDirectionImageAxis];
      
  // Set the region of the output slice
  outputPtr->SetLargestPossibleRegion(outputRegion);

  // Set the spacing and origin
  outputPtr->SetSpacing(outputSpacing);
  outputPtr->SetOrigin(outputOrigin);
}

template<class TPixel>
void IRISSlicer<TPixel>
::CallCopyOutputRegionToInputRegion(InputImageRegionType &destRegion,
                                    const OutputImageRegionType &srcRegion)
{
  // Set the size of the region to 1 in the slice direction
  destRegion.SetSize(m_SliceDirectionImageAxis,1);

  // Set the index of the region in that dimension to the number of the slice
  destRegion.SetIndex(m_SliceDirectionImageAxis,m_SliceIndex);

  // Compute the bounds of the input region for the other two dimensions (for 
  // the case when the output region is not equal to the largest possible 
  // region (i.e., we are requesting a partial slice)

  // The size of the region does not depend of the direction of axis 
  // traversal
  destRegion.SetSize(m_PixelDirectionImageAxis,srcRegion.GetSize(0));
  destRegion.SetSize(m_LineDirectionImageAxis,srcRegion.GetSize(1));

  // However, the index of the region does depend on the direction!
  if(m_PixelTraverseForward)
    {
    destRegion.SetIndex(m_PixelDirectionImageAxis,srcRegion.GetIndex(0));
    }
  else
    {
    // This case is a bit trickier.  The axis direction is reversed, so
    // range [i,...,i+s-1] in the output image corresponds to the range
    // [S-(i+s),S-(i+1)] in the input image, where i is the in-slice index, 
    // S is the largest size of the input and s is the requested size of the 
    // output
    destRegion.SetIndex(
      m_PixelDirectionImageAxis,
      GetInput()->GetLargestPossibleRegion().GetSize(m_PixelDirectionImageAxis)
      - (srcRegion.GetIndex(0) + srcRegion.GetSize(0)));
    }

  // Same as above for line index
  if(m_LineTraverseForward)
    {
    destRegion.SetIndex(m_LineDirectionImageAxis,srcRegion.GetIndex(1));
    }
  else
    {    
    destRegion.SetIndex(
      m_LineDirectionImageAxis,
      GetInput()->GetLargestPossibleRegion().GetSize(m_LineDirectionImageAxis)
      - (srcRegion.GetIndex(1) + srcRegion.GetSize(1)));
    }
}

template<class TPixel> 
void IRISSlicer<TPixel>
::GenerateData()
{
  // Here's the input and output
  InputImagePointer  inputPtr = GetInput();
  OutputImagePointer  outputPtr = GetOutput();
  
  // Allocate (why is this necessary?)
  AllocateOutputs();

  // Compute the region in the image for which the slice is being extracted
  InputImageRegionType inputRegion = inputPtr->GetRequestedRegion();

  // Create an iterator that will parse the desired slice in the image
  InputIteratorType it(inputPtr,inputRegion);
  it.SetFirstDirection(m_PixelDirectionImageAxis);
  it.SetSecondDirection(m_LineDirectionImageAxis);

  // Copy the contents using a different method, depending on the axes directions
  // The direction with index one is the order in which the lines are traversed and
  // the direction with index two is the order in which the pixels are traversed.
  if(m_PixelTraverseForward)
    if(m_LineTraverseForward)
      CopySliceLineForwardPixelForward(it,outputPtr);
    else
      CopySliceLineBackwardPixelForward(it,outputPtr);
  else
    if(m_LineTraverseForward)
      CopySliceLineForwardPixelBackward(it,outputPtr);
    else
      CopySliceLineBackwardPixelBackward(it,outputPtr);
}

template<class TPixel> 
void IRISSlicer<TPixel>
::PrintSelf(std::ostream &os, itk::Indent indent) const
{
  Superclass::PrintSelf(os, indent);

  os << indent << "Slice Image Axis: " << m_SliceDirectionImageAxis << std::endl;
  os << indent << "Slice Index: " << m_SliceIndex << std::endl;
  os << indent << "Line Image Axis:  " << m_LineDirectionImageAxis << std::endl;
  os << indent << "Lines Traversed Forward: " << m_LineTraverseForward << std::endl;
  os << indent << "Pixel Image Axis: " << m_PixelDirectionImageAxis << std::endl;
  os << indent << "Pixels Traversed Forward: " << m_PixelTraverseForward << std::endl;
}

// Traverse pixels forward and lines forward
template<class TPixel> 
void IRISSlicer<TPixel>
::CopySliceLineForwardPixelForward(InputIteratorType itImage, 
                                   OutputImageType *outputPtr)
{  
  // Create an simple iterator for the slice
  SimpleOutputIteratorType itSlice(outputPtr,outputPtr->GetRequestedRegion());

  // Position both iterators at the beginning
  itSlice.GoToBegin();
  itImage.GoToBegin();

  // Parse over lines
  while(!itSlice.IsAtEnd())
    {
    // Parse over pixels
    while(!itImage.IsAtEndOfLine())
      {  
      itSlice.Set(itImage.Value());
      ++itSlice;
      ++itImage;
      }

    // Position the image iterator at the next line
    itImage.NextLine();
    }
}

// Traverse pixels forward and lines backward
template<class TPixel> 
void IRISSlicer<TPixel>
::CopySliceLineBackwardPixelForward(InputIteratorType itImage, 
                                    OutputImageType *outputPtr)
{
  // Create an iterator into the slice itself
  OutputIteratorType itSlice(outputPtr,outputPtr->GetRequestedRegion());
  itSlice.SetDirection(0);

  // Go to the last line  
  itSlice.GoToReverseBegin();
  itImage.GoToBegin();
  
  // Parse over lines
  while(!itSlice.IsAtReverseEnd())
    {
    // Position the slice iterator at the first pixel of the line
    itSlice.GoToBeginOfLine();

    // Scan in forward until the end of line
    while(!itSlice.IsAtEndOfLine())
      {
      itSlice.Set(itImage.Value());
      ++itSlice;
      ++itImage;
      }

    // Step over to the previous line
    itSlice.PreviousLine();
    itImage.NextLine();
    }
}

// Traverse pixels backward and lines forward
template<class TPixel> 
void IRISSlicer<TPixel>
::CopySliceLineForwardPixelBackward(InputIteratorType itImage, 
                                    OutputImageType *outputPtr)
{
  // Create an iterator into the slice itself
  OutputIteratorType itSlice(outputPtr,outputPtr->GetRequestedRegion());
  itSlice.SetDirection(0);

  // Go to start of the last line  
  itSlice.GoToBegin();
  itImage.GoToBegin();
  
  // Parse over lines
  while(!itSlice.IsAtEnd())
    {
    // Position the slice iterator at the last pixel of the line
    itSlice.GoToEndOfLine(); --itSlice;
    
    // Scan in backwards until we're past the beginning of the line
    while(!itSlice.IsAtReverseEndOfLine())
      {
      itSlice.Set(itImage.Value());
      --itSlice;
      ++itImage;
      }

    // Step over to the next line
    itSlice.NextLine();
    itImage.NextLine();
    }
}

// Traverse pixels backward and lines backward
template<class TPixel> 
void IRISSlicer<TPixel>
::CopySliceLineBackwardPixelBackward(InputIteratorType itImage, 
                                     OutputImageType *outputPtr)
{  
  // Create an simple iterator for the slice
  SimpleOutputIteratorType itSlice(outputPtr,outputPtr->GetRequestedRegion());

  // Position both iterators at the beginning
  itSlice.GoToReverseBegin();
  itImage.GoToBegin();

  // Parse over lines
  while(!itSlice.IsAtReverseEnd())
    {
    // Parse over pixels
    while(!itImage.IsAtEndOfLine())
      {  
      itSlice.Set(itImage.Value());
      --itSlice;
      ++itImage;
      }

    // Position the image iterator at the next line
    itImage.NextLine();
    }
}

/*
template<class TPixel> 
void IRISSlicer<TPixel>
::CopySlice(InputIteratorType itImage, OutputIteratorType itSlice, 
            int sliceDir, int lineDir)
{
  if (sliceDir > 0 && lineDir > 0)
    {
    itImage.GoToBegin();
    while (!itImage.IsAtEndOfSlice())
      {
      while (!itImage.IsAtEndOfLine())
        {
        itSlice.Set(itImage.Value());
        ++itSlice;
        ++itImage;
        }
      itImage.NextLine();
      }
    } 
  else if (sliceDir > 0 && lineDir < 0)
    {
    itImage.GoToBegin();
    while (!itImage.IsAtEndOfSlice())
      {
      itImage.NextLine();
      itImage.PreviousLine();
      while (1)
        {
        itSlice.Set(itImage.Value());
        ++itSlice;
        if (itImage.IsAtReverseEndOfLine())
          break;
        else
          --itImage;
        }
      itImage.NextLine();
      }
    } 
  else if (sliceDir < 0 && lineDir > 0)
    {
    itImage.GoToReverseBegin();
    while (!itImage.IsAtReverseEndOfSlice())
      {
      itImage.PreviousLine();
      itImage.NextLine();
      while (!itImage.IsAtEndOfLine())
        {
        TPixel px = itImage.Value();
        itSlice.Set(px);
        ++itSlice;
        ++itImage;
        }
      itImage.PreviousLine();
      } 
    } 
  else if (sliceDir < 0 && lineDir < 0)
    {
    itImage.GoToReverseBegin();
    while (1)
      {
      while (1)
        {
        itSlice.Set(itImage.Value());
        ++itSlice;
        if (itImage.IsAtReverseEndOfLine())
          break;
        else
          --itImage;
        }            
      if (itImage.IsAtReverseEndOfSlice())
        break;
      else
        itImage.PreviousLine();
      } 
    }
}
*/
