/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    IRISException.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __IRIS_Exceptions_h_
#define __IRIS_Exceptions_h_

#include <iostream>
#include <string>

using std::string;

/** 
 * \class IRISException
 * \brief Sets up a family of SNAP/IRIS exceptions
 */
class IRISException {
protected:
  string m_SimpleMessage;

public:
  IRISException();
  IRISException(const char *message);

  virtual ~IRISException();

  operator const char *();
};

/**
 * Set macro borrowed from VTK and modified.  Assumes m_ for private vars
 */
#define irisExceptionMacro(name,parent) \
class name : public parent { \
public: \
        name() : parent() {} \
          name(const char *message) : parent(message) {} \
          virtual ~name() {} \
};

irisExceptionMacro(IRISExceptionIO,IRISException);

#endif
