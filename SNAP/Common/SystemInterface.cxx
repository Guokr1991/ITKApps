/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    SystemInterface.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#include "SystemInterface.h"
#include "IRISApplication.h"
#include "GlobalState.h"
#include "SNAPRegistryIO.h"
#include "FL/Fl_Preferences.h"
#include "FL/filename.h"
#include <itksys/SystemTools.hxx>
#include <algorithm>
#include <ctime>
#include <iomanip>

using namespace std;

SystemInterface
::SystemInterface()
{
  m_RegistryIO = new SNAPRegistryIO;
}

SystemInterface
::~SystemInterface()
{
  delete m_RegistryIO;
}


void
SystemInterface
::SaveUserPreferences()
{
  WriteToFile(m_UserPreferenceFile.c_str());
}


void
SystemInterface
::LoadUserPreferences()
{
  // Create a preferences object
  Fl_Preferences test(Fl_Preferences::USER,"itk.org","SNAP");
  
  // Use it to get a path for user data
  char userDataPath[1024]; 
  test.getUserdataPath(userDataPath,1024);
  
  // Construct a valid path
  m_UserPreferenceFile = string(userDataPath) + "/" + "UserPreferences.txt";

  // Check if the file exists, may throw an exception here
  if(itksys::SystemTools::FileExists(m_UserPreferenceFile.c_str()))
  {
    ReadFromFile(m_UserPreferenceFile.c_str());
  }
}

/** 
 * A method that checks whether the SNAP system directory can be found and 
 * if it can't, prompts the user for the directory.  The path parameter is the
 * location of the executable, i.e., argv[0] 
 */
bool 
SystemInterface
::FindDataDirectory(const char *pathToExe)
{
  // Get the directory where the SNAP executable was launched
  using namespace itksys;
  typedef std::string StringType;

  // This is the directory we're trying to set
  StringType sRootDir = "";

  // The first possibility is that the user has specified the data path
  // manually.  Get the path from the registry
  StringType sUserPath = (*this)["System.ProgramDataDirectory"][""];
  StringType sSearchName = sUserPath  + "/" + GetProgramDataDirectoryTokenFileName();
  if(sUserPath.length() && 
    SystemTools::FileIsDirectory(sUserPath.c_str()) &&
    SystemTools::FileExists(sSearchName.c_str()))
    {
    // We've found the path
    sRootDir = sUserPath;
    }
  else
    {
    // First of all, find the executable file.  Since the program may have been
    // in the $PATH variable, we don't know for sure where the data is
    // Create a vector of paths that will be searched for 
    // the file SNAPProgramDataDirectory.txt
    vector<StringType> vPathList;

    // Nevertheless, we'll search for the SNAP executable
    StringType sExeFullPath = SystemTools::FindProgram(pathToExe);

    // If the exe could not be found, we can't use it to construct a list of
    // potential search program data search paths
    if(sExeFullPath.length())
      {
      // Look in the path right off the EXE
      StringType sExePath = SystemTools::GetFilenamePath(sExeFullPath);
      if(sExePath.length())
        {
        vPathList.push_back(sExePath + "/" + "ProgramData");
        }

      // Look in the path just above the EXE (windows)
      StringType sExePathUp = SystemTools::GetFilenamePath(sExePath);
      if(sExePathUp.length()) 
        {
        vPathList.push_back(sExePathUp + "/" + "ProgramData");
        }
      }

    // Now that we have the path list, search for the token file
    StringType sFoundFile = 
      SystemTools::FindFile(GetProgramDataDirectoryTokenFileName(),vPathList);
    if(sFoundFile.length())
      sRootDir = SystemTools::GetFilenamePath(sFoundFile);
    }

  // If a directory was not found, prompt the user
  if(!sRootDir.length())
    return false;

  // Store the property, so the next time we don't have to search at all
  (*this)["System.ProgramDataDirectory"] << sRootDir;
  
  // Set the root directory and relative paths
  m_DataDirectory = sRootDir;

  // Append the paths to get the other directories
  m_DocumentationDirectory = m_DataDirectory + "/HTMLHelp";

  // Done, success
  return true;
}

std::string
SystemInterface
::GetFileInRootDirectory(const char *fnRelative)
{
  // Construct the file name
  string path = m_DataDirectory + "/" + fnRelative;

  // Make sure the file exists ?

  // Return the file
  return path;
}

bool 
SystemInterface
::FindRegistryAssociatedWithFile(const char *file, Registry &registry)
{
  // Convert the file to an absolute path
  char buffer[1024];
  fl_filename_absolute(buffer,1024,file);

  // Convert to unix slashes for consistency
  string path(buffer);
  itksys::SystemTools::ConvertToUnixSlashes(path);

  // Convert the filename to a numeric string (to prevent clashes with the Registry class)
  path = EncodeFilename(path);

  // Get the key associated with this filename
  string key = Key("ImageAssociation.Mapping.Element[%s]",path.c_str());
  string code = Entry(key)[""];

  // If the code does not exist, return w/o success
  if(code.length() == 0) return false;

  // Create a preferences object for the associations subdirectory
  Fl_Preferences test(Fl_Preferences::USER,"itk.org","SNAP/ImageAssociations");

  // Use it to get a path for user data
  char userDataPath[1024]; 
  test.getUserdataPath(userDataPath,1024);

  // Create a save filename
  IRISOStringStream sfile;
  sfile << userDataPath << "/" << "ImageAssociation." << code << ".txt";

  // Try loading the registry
  try 
    {
    registry.ReadFromFile(sfile.str().c_str());
    return true;
    }
  catch(...)
    {
    return false;
    }
}

string
SystemInterface
::EncodeFilename(const string &src)
{
  IRISOStringStream sout;
  
  for(unsigned int i=0;i<src.length();i++)
    sout << setw(2) << setfill('0') << hex << (int)src[i];

  return sout.str();
}

bool 
SystemInterface
::AssociateRegistryWithFile(const char *file, Registry &registry)
{
  // Convert the file to an absolute path
  char buffer[1024];
  fl_filename_absolute(buffer,1024,file);

  // Convert to unix slashes for consistency
  string path(buffer);
  itksys::SystemTools::ConvertToUnixSlashes(path);
  path = EncodeFilename(path);

  // Compute a timestamp from the start of computer time
  time_t timestr = time(NULL);

  // Create a key for the file
  IRISOStringStream scode;
  scode << setfill('0') << setw(16) << hex << timestr;
  
  // Look for the file in the registry (may already exist, if not use the
  // code just generated
  string key = Key("ImageAssociation.Mapping.Element[%s]",path.c_str());
  string code = Entry(key)[scode.str().c_str()];
  
  // Put the key in the registry
  Entry(key) << code;

  // Create a preferences object for the associations subdirectory
  Fl_Preferences test(Fl_Preferences::USER,"itk.org","SNAP/ImageAssociations");

  // Use it to get a path for user data
  char userDataPath[1024]; 
  test.getUserdataPath(userDataPath,1024);

  // Create a save filename
  IRISOStringStream sfile;
  sfile << userDataPath << "/" << "ImageAssociation." << code << ".txt";

  // Store the registry to that path
  try 
    {
    registry.WriteToFile(sfile.str().c_str());
    return true;
    }
  catch(...)
    {
    return false;
    }  
}

bool 
SystemInterface
::AssociateCurrentSettingsWithCurrentImageFile(const char *file, IRISApplication *app)
{
  // Get a registry already associated with this filename
  Registry registry;
  FindRegistryAssociatedWithFile(file,registry);

  // Write the current state into that registry
  m_RegistryIO->WriteImageAssociatedSettings(app,registry);

  // Write the registry back
  return AssociateRegistryWithFile(file,registry);
}

bool 
SystemInterface
::RestoreSettingsAssociatedWithImageFile(
  const char *file, IRISApplication *app,
  bool restoreLabels, bool restorePreprocessing,
  bool restoreParameters, bool restoreDisplayOptions)
{
  // Get a registry already associated with this filename
  Registry registry;
  if(FindRegistryAssociatedWithFile(file,registry))
    {
    m_RegistryIO->ReadImageAssociatedSettings(
      registry, app,
      restoreLabels, restorePreprocessing,
      restoreParameters, restoreDisplayOptions);
    return true;
    }
  else
    return false;
}

/** Get a filename history list by a particular name */
SystemInterface::HistoryListType 
SystemInterface
::GetHistory(const char *key)
{
  // Get the history array
  return Folder("IOHistory").Folder(string(key)).GetArray(string(""));
}

/** Update a filename history list with another filename */
void
SystemInterface
::UpdateHistory(const char *key, const char *filename)
{
  // Create a string for the new file
  string file(filename);

  // Get the current history registry
  HistoryListType array = GetHistory(key);

  // First, search the history for the instance of the file and delete
  // existing occurences
  HistoryListType::iterator it;
  while((it = find(array.begin(),array.end(),file)) != array.end())
    array.erase(it);

  // Append the file to the end of the array
  array.push_back(file);

  // Trim the array to appropriate size
  if(array.size() > 20)
    array.erase(array.begin(),array.begin() + array.size() - 20);

  // Store the new array to the registry
  Folder("IOHistory").Folder(string(key)).PutArray(array);      
}



