#include <stdio.h>
#include <AquesTalk2Da.h>
#define AqKanji2Koe_Create _AqKanji2Koe_Create
#define AqKanji2Koe_Convert _AqKanji2Koe_Convert
#define AqKanji2Koe_Release _AqKanji2Koe_Release
#include <AqKanji2Koe.h>

#include <glib.h>

char *phont_files[] = {
  "./phont/aq_f1c.phont",
  "./phont/aq_f3a.phont",
  "./phont/aq_huskey.phont",
  "./phont/aq_m4b.phont",
  "./phont/aq_mf1.phont",
  "./phont/aq_rb2.phont",
  "./phont/aq_rb3.phont",
  "./phont/aq_rm.phont",
  "./phont/aq_robo.phont",
  "./phont/aq_yukkuri.phont",
};

int main(int argc, char *argv[])
{
  int size;
  int nErr=0;
  char buf[1024];
  char *phont_path = NULL;
  
  if (argc == 2) {
    GRand *g_rnd = g_rand_new();
    int nphont = g_rand_int_range(g_rnd, 0, 10);
    g_print("%d\n", nphont);
    phont_path = phont_files[nphont];
  } else if (argc == 3){
    phont_path = argv[2];
  }else {
    printf ("HelloTalk [talk string] [phont file path]");
    return 0;
  }

  void* p=AqKanji2Koe_Create("./aq_dic", &nErr);
  if (p==NULL){
    printf("error\n");
  }
  AqKanji2Koe_Convert(p, argv[1], buf, 1024);
  printf("%s\n", buf);

  
  GError *perr = NULL;
  GMappedFile *gmap=NULL;
  g_print("%s\n", phont_path);
  gmap = g_mapped_file_new(phont_path, FALSE, &perr);
  if (gmap!=NULL){
    gchar *pc = g_mapped_file_get_contents(gmap);
    AquesTalk2Da_PlaySync(buf, 100, pc);
    g_mapped_file_free(gmap);
  }

  AqKanji2Koe_Release(p);
  return 0;
}
