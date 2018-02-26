#include<iostream>
#include "CImg.h"


using namespace std;
using namespace cimg_library;
#define ALPHA_THRESHOLD 0.7
#define SIZE_OF_BLOCKS 10


//calculates the average,maximum and minimum brightness
void calcAverages(CImg<unsigned char>& image, double& Xmin,double& Xmax,double& Xavg){
    Xmin=255.0;
    Xmax=0.0;
    int totalBrightnessSum=0;
    for(int i=0;i<image.width()-1;i++){
        for(int j=0;j<image.height()-1;j++){
        //get R,G,B values
        int R=(int)image(i,j,0,0),G=(int)image(i,j,0,1),B=(int)image(i,j,0,2);
        //calculate brightness as Average of the three
        double brightness=(double)(R+G+B)/3;
        //update total sum, min and max value if needed
        totalBrightnessSum+=brightness;
        if(brightness<=Xmin)Xmin=brightness;
        if(brightness>=Xmax)Xmax=brightness;
        }
    }
    Xavg=totalBrightnessSum/(image.width()*(double)image.height());
}

//calculates the membership function for the fuzzy set
double membershipFunc(int x, double Xmin, double Xmax, double Xavg){
    double Dl= Xavg-Xmin,  Dr=Xmax-Xavg;
    double memValue=0;
    if(x<=Xmin || x>=Xmax)memValue=0;
    else if(x>Xavg)memValue=(Xmax-x)/Dr;
    else if(x<Xavg)memValue=(x-Xmin)/Dl;
    else if(x==Xavg)memValue=1;
    return memValue;
}

//calculates membership functions for all pixels and does the alpha cut
void calculateObviousRegions(CImg<unsigned char>& image,double Xmin, double Xmax, double Xavg){
    for(int i=0;i<image.width();i++){
        for(int j=0;j<image.height();j++){

        int R=(int)image(i,j,0,0),G=(int)image(i,j,0,1),B=(int)image(i,j,0,2);
        int brightness=(R+G+B)/3;
        int x=brightness;
        double memVal=membershipFunc(brightness,Xmin,Xmax,Xavg);
        //cout<<"memval for ("<<i<<","<<j<<") is: "<<memVal<<"   ......    ";
        const unsigned char black[] = {0,0,0};
        const unsigned char white[] = {255,255,255};

        if(memVal<=ALPHA_THRESHOLD){

//            if(x<(Xmin + Dl/2))image.draw_point(i,j,black);
//            if(x>(Xavg + Dr/2))image.draw_point(i,j,white);

            if(x<Xavg)image.draw_point(i,j,black);
            if(x>Xavg)image.draw_point(i,j,white);
            }
        }
    }


}

//calculates the temporary threshold of bloc (m,n)
double temporaryThreshhold(CImg<unsigned char>& image,int m,int n){
    double totalThreshhold;
    for(int x=0;x<SIZE_OF_BLOCKS;x++){
        for(int y=0;y<SIZE_OF_BLOCKS;y++){
            int i=m*SIZE_OF_BLOCKS+x;
            int j=n*SIZE_OF_BLOCKS+y;

            int R=(int)image(i,j,0,0),G=(int)image(i,j,0,1),B=(int)image(i,j,0,2);
            int brightness=(R+G+B)/3;

            totalThreshhold+=brightness;
        }
    }
    return totalThreshhold/(SIZE_OF_BLOCKS*SIZE_OF_BLOCKS);
}

//given a threshold - binarizes block (m,n) of the image
void binarizeRegion(CImg<unsigned char>& image,int m,int n,double threshold){
    for(int x=0;x<SIZE_OF_BLOCKS;x++){
        for(int y=0;y<SIZE_OF_BLOCKS;y++){
            int i=m*SIZE_OF_BLOCKS+x;
            int j=n*SIZE_OF_BLOCKS+y;

            int R=(int)image(i,j,0,0),G=(int)image(i,j,0,1),B=(int)image(i,j,0,2);
            int brightness=(R+G+B)/3;

            const unsigned char black[] = {0,0,0};
            const unsigned char white[] = {255,255,255};
            //cout<<"current i and j and threshold :" <<i<<"   "<<j<< " ,threshold:"<<threshold<<endl;
            if(brightness>=threshold)image.draw_point(i,j,white);
            else image.draw_point(i,j,black);
        }
    }
}

//returns the weighted threshold of a block with regards to it's neighbours and the Xavg
double getWeightedThreshold(double& Xavg,int m,int n,vector<vector<double>>& temporaryThresholds,int width,int height){
//    cout<<"calculating for : ("<<m<<","<<n<<")"<<"num of blocks is:"<<width<<"  "<<height<<endl;
    if(m==0 && n==0)return (Xavg*2 + temporaryThresholds[m][n]*4 + temporaryThresholds[m][n+1]*2+
        temporaryThresholds[m+1][n]*2)/10;
    if(m==0 && n>0 )return (Xavg*2 + temporaryThresholds[m][n]*4 + temporaryThresholds[m][n+1]+ temporaryThresholds[m][n-1]+
        temporaryThresholds[m+1][n]*2)/10;

    if(m==width-1 && n==height-1)return (Xavg*2 + temporaryThresholds[m][n]*4 + temporaryThresholds[m-1][n]*2 +
        temporaryThresholds[m][n-1]*2)/10;
    if(m==width-1 && n<height-1)return (Xavg*2 + temporaryThresholds[m][n]*4 + temporaryThresholds[m][n-1] +
        temporaryThresholds[m][n+1] + temporaryThresholds[m-1][n]*2)/10;
    if(n==height-1 && m<width-1)return (Xavg*2 + temporaryThresholds[m][n]*4 + temporaryThresholds[m-1][n] + temporaryThresholds[m+1][n]+
        temporaryThresholds[m][n-1]*2)/10;
    if(m>0 && n==0 ){
    return (Xavg*2 + temporaryThresholds[m][n]*4 + temporaryThresholds[m][n+1]*2 +
        temporaryThresholds[m+1][n]+ temporaryThresholds[m-1][n])/10;
    }
    return (Xavg*2+temporaryThresholds[m][n]*4 + temporaryThresholds[m][n+1]+
        temporaryThresholds[m][n-1]+ temporaryThresholds[m+1][n]+ temporaryThresholds[m-1][n])/10;


}

//script that calculates all thresholds for blocks and binarizes them
void binarizeWithThresholds(CImg<unsigned char>& image, double& Xavg,int& numBlocksOnColumn,int& numBlocksOnRow,
vector<vector<double>>& temporaryThresholds){

    for(int m=0;m<numBlocksOnColumn;m++){
        for(int n=0;n<numBlocksOnRow;n++){
        double newThreshold=getWeightedThreshold(Xavg,m,n,temporaryThresholds,numBlocksOnColumn,numBlocksOnRow);
//        cout<<"threshold for: ("<<m<<","<<n<<") is: "<<newThreshold<<endl;
//         cout<<"got to here"<<endl;
        binarizeRegion(image,m,n,newThreshold);
        }
    }


}



//shows the brightness of all points in the image on a graph
void showGraphFromBrightness(CImg<unsigned char> image,double Xavg=0,double Xmax=0,double Xmin=0){
    CImg<unsigned char> visu(2000,500,1,3,0);
    CImgDisplay graph_disp(visu,"Graph representation");
    const unsigned char graph_color[] = { 255,0,0 };
    const unsigned char line_color[] = { 0,0,0 };
    const unsigned char avg_color[] = { 255,165,0};

    visu.fill(255);
    int width = visu.width();
    int height = visu.height();
    int center_X=30,center_Y=height-height/4;
    visu.draw_line(center_X,0,center_X,height,line_color);
    visu.draw_line(0,center_Y,width,center_Y,line_color);
     for(int i=0;i<image.width();i++){
        for(int j=0;j<image.height();j++){

            int R=(int)image(i,j,0,0),G=(int)image(i,j,0,1),B=(int)image(i,j,0,2);
            double brightness=(double)(R+G+B)/3;
            visu.draw_line(center_X+i*width+j,center_Y,center_X+i*width+j,center_Y-brightness,graph_color);

            }
    }
    //avg
    visu.draw_line(center_X,center_Y-Xavg,visu.width(),center_Y-Xavg,avg_color);
    //max
    visu.draw_line(center_X,center_Y-Xmax,visu.width(),center_Y-Xmax,avg_color);
    //min
    visu.draw_line(center_X,center_Y-Xmin,visu.width(),center_Y-Xmin,avg_color);


    while(!graph_disp.is_closed()){
    visu.display(graph_disp);
}

    graph_disp.wait();

}





int main(){

    CImg<unsigned char> src("someIce.jpeg");
    int width = src.width();
    int height = src.height();
    cout << width << "x" << height << endl;
    double Xmin,Xmax,Xavg;
    calcAverages(src,Xmin,Xmax,Xavg);

//  code to visualise the pixel brightness of the image on a graph
//  showGraphFromBrightness(src,Xavg,Xmax,Xmin);



    //create new image copy to draw on
    CImg<unsigned char> newImg(src,0);
    //do alpha-cut and binarize
    calculateObviousRegions(newImg,Xmin,Xmax,Xavg);
    //go through image and instantiate temporary thresholds for all squares
    int numBlocksOnRow=height/SIZE_OF_BLOCKS;
    int numBlocksOnColumn=width/SIZE_OF_BLOCKS;
    int newWidth=numBlocksOnColumn*SIZE_OF_BLOCKS;
    int newHeight=numBlocksOnRow*SIZE_OF_BLOCKS;
    newImg=newImg.get_crop(0,0,newWidth-1,newHeight-1);
    //create thresholds matrix
    vector<vector<double>> temporaryThresholds;
    for(int m=0;m<numBlocksOnColumn;m++){
        vector<double> rowM;
        for(int n=0;n<numBlocksOnRow;n++){
        //calculate threshold and push it into the thresholds matrix
        rowM.push_back(temporaryThreshhold(newImg,m,n));
        //temporaryThresholds[m][n]=temporaryThreshhold(newImg,m,n);
        //cout<<"for block ("<<m<<","<<n<<") threshold is = "<<temporaryThresholds[m][n]<<endl;
        }
        temporaryThresholds.push_back(rowM);
    }
    //after finding the temporary thresholds, proceed with calculating average thresholds and binarizing locally
   binarizeWithThresholds(newImg,Xavg,numBlocksOnColumn,numBlocksOnRow,temporaryThresholds);
   //save the image to a file
   newImg.save("finalizedAlphas/finalIce07threshold.jpg");

   return 0;
}




