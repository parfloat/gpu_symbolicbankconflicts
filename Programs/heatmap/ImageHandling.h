#ifndef IMAGEHANDLING_H_
#define IMAGEHANDLING_H_

#include <stdio.h>

void writeIMG(char* imgName, int *red, int *green, int *blue, unsigned long int w, unsigned long int h, unsigned long int size){
  FILE *f;
  unsigned char *img = NULL;
  long int filesize = 54 + 3*w*h;  //w is your image width, h is image height, both int

  img = (unsigned char *)malloc(3*w*h);
  memset(img,0,3*w*h);

  // printf("size = %ld\n", size);
  for(unsigned long int i=0; i<w; i++)
  {
      for(unsigned long int j=0; j<h; j++)
      {
          int x=i; int y=(h-1)-j;
          // printf("j * h + i => %ld * %ld + %ld = %ld\n", j, h, i, (j * h + i));
          int r = red[j * h + i];//red[i * w + j];//rand() % 256;//red[i][j]*255;
          int g = green[j * h + i];//green[i * w + j];//rand() % 256;//green[i][j]*255;
          int b = blue[j * h + i];//blue[i * w + j];//rand() % 256;//blue[i][j]*255;
          if (r > 255) r=255;
          if (g > 255) g=255;
          if (b > 255) b=255;
          img[(x+y*w)*3+2] = (unsigned char)(r);
          img[(x+y*w)*3+1] = (unsigned char)(g);
          img[(x+y*w)*3+0] = (unsigned char)(b);
      }
  }

  unsigned char bmpfileheader[14] = {'B','M', 0,0,0,0, 0,0, 0,0, 54,0,0,0};
  unsigned char bmpinfoheader[40] = {40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0, 24,0};
  unsigned char bmppad[3] = {0,0,0};

  bmpfileheader[ 2] = (unsigned char)(filesize    );
  bmpfileheader[ 3] = (unsigned char)(filesize>> 8);
  bmpfileheader[ 4] = (unsigned char)(filesize>>16);
  bmpfileheader[ 5] = (unsigned char)(filesize>>24);

  bmpinfoheader[ 4] = (unsigned char)(       w    );
  bmpinfoheader[ 5] = (unsigned char)(       w>> 8);
  bmpinfoheader[ 6] = (unsigned char)(       w>>16);
  bmpinfoheader[ 7] = (unsigned char)(       w>>24);
  bmpinfoheader[ 8] = (unsigned char)(       h    );
  bmpinfoheader[ 9] = (unsigned char)(       h>> 8);
  bmpinfoheader[10] = (unsigned char)(       h>>16);
  bmpinfoheader[11] = (unsigned char)(       h>>24);

  f = fopen(imgName,"wb");
  fwrite(bmpfileheader,1,14,f);
  fwrite(bmpinfoheader,1,40,f);
  for(int i=0; i<h; i++)
  {
      fwrite(img+(w*(h-i-1)*3),3,w,f);
      fwrite(bmppad,1,(4-(w*3)%4)%4,f);
  }

  free(img);
  fclose(f);

}



#endif
