#include <stdio.h>
#include <dirent.h>
#include <iostream>
#include <fstream>


int main()
{
   DIR *dirlist;
   struct dirent *dentry;
   std::string convertname;
   
   bool test = std::ifstream("linvst3.so").good();
   
   if(!test)
   {
      printf("can't find linvst3.so file\n");
      return 0;
   }
   
   dirlist = opendir("."); 
   
   if(dirlist != NULL)
   {
      
      while((dentry = readdir(dirlist)) != NULL)
      {
         
         convertname = " ";
         
         convertname = dentry->d_name;
         
         if(convertname.find(".vst3") != std::string::npos)
         {
            convertname.replace(convertname.begin() + convertname.find(".vst3"), convertname.end(), ".so");
         }
         else if(convertname.find(".Vst3") != std::string::npos)
         {
            convertname.replace(convertname.begin() + convertname.find(".Vst3"), convertname.end(), ".so");
         }
         else if(convertname.find(".VST3") != std::string::npos)
         {
            convertname.replace(convertname.begin() + convertname.find(".VST3"), convertname.end(), ".so");
         }
         else
            continue;
         
         std::ifstream source("linvst3.so", std::ios::binary);
         
         std::ofstream dest(convertname.c_str(), std::ios::binary);
         
         dest << source.rdbuf();
         
         source.close();
         dest.close();
         
      }
      
   }
   
   closedir(dirlist);
   
}

