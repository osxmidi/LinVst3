   #include <stdio.h>
   #include <dirent.h>
   #include <iostream>
   #include <fstream>
   #include <gtk/gtk.h>
   #include <string.h>
   #include <fts.h>
   #include <sys/stat.h>   
   #include <bits/stdc++.h>
    
   gchar *folderpath;
   gchar *filepath;

   int filehit = 0;
   int folderhit = 0;
   int filecopy = 0;

   int intimer = 0;

   int dofullup = 0;
   
   char foldertest[2048];

   size_t find_last(std::string searchstr, std::string searcharg)
   {
   size_t foundret = 0;
   size_t found = searchstr.find(searcharg, 0);
   foundret = found;
 
   while(found != std::string::npos)
   {
   found += searcharg.size();
   found = searchstr.find(searcharg, found);
   if(found != std::string::npos)
   foundret = found;
   }
   return foundret;
   }
  
   bool dosym(std::string path)
   {
    std::string vst3string;
    std::string vst3string2;  
    std::string vst3stringcase;  
    struct stat sb;  
    size_t found;
      
	vst3string = path;
    vst3stringcase = path; 
    transform(vst3stringcase.begin(), vst3stringcase.end(), vst3stringcase.begin(), ::tolower);
    found = vst3stringcase.find(".vst3", 0);
    vst3string2 = vst3string.substr(0, found + 5);
    vst3string = vst3string.substr(0, found);  
    if((stat(vst3string2.c_str(), &sb) == 0) && S_ISDIR(sb.st_mode))
	{
    vst3string = vst3string + "-linvst3";    
    symlink(vst3string2.c_str(), vst3string.c_str());  
    return true;
    }
    return false;
    } 

    int doconvert(char *linvst, char folder[])
    { 
    DIR *dirlist;
    struct dirent *dentry;
    std::string convertname;
    std::string convertnamecase;
    std::string cfolder;
    char *folderpath[] = {folder, 0};
    int vst3filehit; 
    FTSENT *node;  
    FTS *fs;
    bool test;

    filecopy = 1;  
    test = std::ifstream(linvst).good();
   
    if(!test)
    {
    filecopy = 0;
    return 1;
    }

    fs = fts_open(folderpath, FTS_NOCHDIR | FTS_LOGICAL, 0);
    if (!fs) 
    {
    return 1;
    }

    while ((node = fts_read(fs))) 
    {      
    switch (node->fts_info) {
 	case FTS_DNR:	
	case FTS_ERR:
	case FTS_NS:
	case FTS_DP: 
	break;
	case FTS_F:
	convertname = node->fts_path;
    convertnamecase = convertname;
    transform(convertnamecase.begin(), convertnamecase.end(), convertnamecase.begin(), ::tolower);
	vst3filehit = 0;

    if(convertnamecase.find(".vst3") != std::string::npos)
	{
    int fulllength = strlen(convertnamecase.c_str());
    if((convertnamecase[fulllength - 1] == '3') && (convertnamecase[fulllength - 2] == 't') && (convertnamecase[fulllength - 3] == 's') && (convertnamecase[fulllength - 4] == 'v') && (convertnamecase[fulllength - 5] == '.'))
    {    
    dosym(convertname.c_str());
    size_t position = find_last(convertnamecase, ".vst3");
    if(position != 0)
    convertname.replace(convertname.begin() + position, convertname.end(), ".so");
    vst3filehit = 1;
	}	
    }

	if(vst3filehit == 1)
	{
    if(dofullup == 1)
    {
    std::string sourcename = linvst;
    std::ifstream source(sourcename.c_str(), std::ios::binary);      
    std::ofstream dest(convertname.c_str(), std::ios::binary);
    dest << source.rdbuf();
    source.close();
    dest.close();	
    }
    else
    {
    test = std::ifstream(convertname).good();
   
    if(!test)
    {
    std::string sourcename = linvst;
    std::ifstream source(sourcename.c_str(), std::ios::binary);      
    std::ofstream dest(convertname.c_str(), std::ios::binary);
    dest << source.rdbuf();
    source.close();
    dest.close();	
	}	
    }
    }		
	break;

    default:
	break;
    }	
    }
	
    if(fs)
    fts_close(fs);
    filecopy = 0;

    return 0;    
   }

void quitcallback ()
{
  if(filecopy == 0)
{
  if(folderpath)
  g_free(folderpath);

  if(filepath)
  g_free(filepath);

  gtk_main_quit ();
}
}

void foldercallback (GtkFileChooser *folderselect)
{

folderpath = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (folderselect));

folderhit = 1;

}

void filecallback (GtkFileChooser *fileselect)
{

filepath = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (fileselect));

filehit = 1;

}

gboolean dolabelupdate(gpointer data)
{

 gtk_button_set_label(GTK_BUTTON (data), "Update Newly Added Plugins");

 intimer = 0;

 return FALSE;

}

gboolean dolabelupdate2(gpointer data)
{

 gtk_button_set_label(GTK_BUTTON (data), "Update All Plugins (Upgrade All Plugins)");

 intimer = 0;

 return FALSE;

}

void buttoncallback(GtkFileChooser *button, gpointer data)
{

 std::string name;

 if((filehit == 1) && (folderhit == 1) && (intimer == 0))
 {
 
 name = filepath;
 
 if(name.find("linvst3.so") == std::string::npos)
 {
 gtk_button_set_label(GTK_BUTTON (button), "Not Found");

 filecopy = 0;
 filehit = 0;
 folderhit = 0;

 intimer = 1;

 g_timeout_add_seconds(3, dolabelupdate, (GtkWidget *)data);

 return;
 }

 if(doconvert(filepath, folderpath) == 1)
 {
 gtk_button_set_label(GTK_BUTTON (button), "Not Found");

 filecopy = 0;
 filehit = 0;
 folderhit = 0;

 intimer = 1;

 g_timeout_add_seconds(3, dolabelupdate, (GtkWidget *)data);

 return;
 }
 
 filecopy = 0;
 filehit = 1;
 folderhit = 0;

 gtk_button_set_label(GTK_BUTTON (button), "Done");

 intimer = 1;

 g_timeout_add_seconds(3, dolabelupdate, (GtkWidget *)data);

}

}

void buttoncallback2(GtkFileChooser *button, gpointer data)
{
 std::string name;

 if((filehit == 1) && (folderhit == 1) && (intimer == 0))
 {
 
 name = filepath;
 
 if(name.find("linvst3.so") == std::string::npos)
 {
 gtk_button_set_label(GTK_BUTTON (button), "Not Found");

 filecopy = 0;
 filehit = 0;
 folderhit = 0;

 intimer = 1;

 g_timeout_add_seconds(3, dolabelupdate2, (GtkWidget *)data);

 return;
 }

 dofullup = 1;

 if(doconvert(filepath, folderpath) == 1)
 {
 gtk_button_set_label(GTK_BUTTON (button), "Not Found");

 filecopy = 0;
 filehit = 0;
 folderhit = 0;
 dofullup = 0;

 intimer = 1;

 g_timeout_add_seconds(3, dolabelupdate2, (GtkWidget *)data);

 return;
 }
 
 filecopy = 0;
 filehit = 1;
 folderhit = 0;
 dofullup = 0;

 gtk_button_set_label(GTK_BUTTON (button), "Done");

 intimer = 1;

 g_timeout_add_seconds(3, dolabelupdate2, (GtkWidget *)data);

}
}

int main (int argc, char *argv[])
{
  GtkWidget *window, *folderselect, *fileselect, *spacertext, *spacertext2, *spacertext3, *vbox, *button, *button2;
  GtkFileFilter *extfilter;

  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size (GTK_WINDOW (window), 600, 300);	
  gtk_window_set_title (GTK_WINDOW (window), "LinVst3");
  gtk_container_set_border_width (GTK_CONTAINER (window), 8);

  spacertext = gtk_label_new ("Choose linvst3.so");
  spacertext2 = gtk_label_new ("Choose vst3 folder");
  spacertext3 = gtk_label_new ("Convert");
  
  folderselect = gtk_file_chooser_button_new ("Choose vst3 Folder", GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
  fileselect = gtk_file_chooser_button_new ("Choose linvst3.so", GTK_FILE_CHOOSER_ACTION_OPEN);

  button = gtk_button_new ();
  gtk_button_set_label(GTK_BUTTON (button), "Update Newly Added Plugins");

  button2 = gtk_button_new ();
  gtk_button_set_label(GTK_BUTTON (button2), "Update All Plugins (Upgrade All Plugins)");

  vbox = gtk_vbox_new (FALSE, 8);
  gtk_box_pack_start(GTK_BOX (vbox), spacertext, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX (vbox), fileselect, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX (vbox), spacertext2, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX (vbox), folderselect, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX (vbox), spacertext3, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX (vbox), button2, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (quitcallback), NULL);
  g_signal_connect (G_OBJECT (folderselect), "selection_changed", G_CALLBACK (foldercallback), NULL);
  g_signal_connect (G_OBJECT (fileselect), "selection_changed", G_CALLBACK (filecallback), NULL);
  g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (buttoncallback), button);
  g_signal_connect (G_OBJECT (button2), "clicked", G_CALLBACK (buttoncallback2), button2);

  gtk_file_chooser_set_show_hidden(GTK_FILE_CHOOSER(folderselect), TRUE);
  gtk_file_chooser_set_show_hidden(GTK_FILE_CHOOSER(fileselect), TRUE);

  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (folderselect), g_get_home_dir());
  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (fileselect), g_get_current_dir ());

  extfilter = gtk_file_filter_new ();
  gtk_file_filter_add_pattern (extfilter, "linvst3.so");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (fileselect), extfilter);

  gtk_container_add (GTK_CONTAINER (window), vbox);
  gtk_widget_show_all (window);

  gtk_main ();
  return 0;
}

