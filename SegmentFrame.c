/*
 * SegmentFrame.c
 *
 *  Created on: Aug 25, 2009
 *      Author: andy
 *
 *   Based in large part on the example on p.242 of the O'Reilly Book: Learning OpenCV
 */

//Adding a comment.


#include <stdio.h>

//OpenCV Headers
#include <highgui.h>
#include <cv.h>

//Andy's Personal Headers
#include "MyLibs/AndysOpenCVLib.h"
#include "MyLibs/WormAnalysis.h"


//C Libraries
#include <string.h>
#include <math.h>



//Global Variables
/* Create a new instance of the WormAnalysis Data structure */
WormAnalysisData* Worm;
WormAnalysisParam* Params;
Frame* IlluminationFrame;
WormGeom* PrevWorm;
int DISPVID;


void on_trabckar(int);



void on_trackbar(int){
	RefreshWormMemStorage(Worm);
	FindWormBoundary(Worm,Params);

	cvDrawContours(Worm->ImgSmooth, Worm->Boundary, cvScalar(255,0,0),cvScalar(0,255,0),100);
	if (DISPVID) cvShowImage("Original",Worm->ImgOrig);
	if (DISPVID) cvShowImage("Thresholded",Worm->ImgThresh);
	//cvWriteFrame(Vid1,Worm->ImgThresh);



	if (GivenBoundaryFindWormHeadTail(Worm,Params)<0){
		printf("Error FindingWormHeadTail!\n");
	}

	/** If we are doing temporal analysis, improve the WormHeadTail estimate based on prev frame **/
	if (Params->TemporalOn){
		PrevFrameImproveWormHeadTail(Worm,Params,PrevWorm);
	}

	SegmentWorm(Worm,Params);


	//Draw a circle on the tail.
	  DisplayWormHeadTail(Worm,"Boundary");

	if (DISPVID)   DisplayWormSegmentation(Worm,"Contours");

	//DisplaySegPts(Worm,"Boundary");

	/** Illuminate the Worm**/
	if (SimpleIlluminateWorm(Worm,IlluminationFrame,2,3)==0) cvShowImage("ToDLP",IlluminationFrame->iplimg);


	/** Update PrevWorm Info **/
	LoadWormGeom(PrevWorm,Worm);



}





int main (int argc, char** argv){
	DISPVID=1;
	/* This will let us know if  the intel primitives are installed*/
	DisplayOpenCVInstall();
	Worm=CreateWormAnalysisDataStruct();
	Params=CreateWormAnalysisParam();

	/** Choose only 20 segments **/
	Params->NumSegments=20;

	CvCapture* capture;
	IplImage* tempImg;

	printf("This program reads in an avi, finds a worm, and segments it.");
	if( argc != 2  ) return -1;
	capture = cvCreateFileCapture(argv[1]);

	/*
	 * Load in the first image to get the size of the image
	 *
	 */

	tempImg=cvQueryFrame(capture);
	if (tempImg==NULL) printf("There was an error querying the frame!\n");

	/*
	 * Fill up the Worm structure with Emtpy Images
	 */
	InitializeEmptyWormImages(Worm,cvGetSize(tempImg));
	InitializeWormMemStorage(Worm);

	/*
	 * Allocate memory for IlluminationFrame
	 */
	 IlluminationFrame=CreateFrame(cvGetSize(tempImg));

	 /*
	  * Allocate Memory for PrevWorm
	  */

	 PrevWorm=CreateWormGeom();

	/*
	 * Load in the Color Source Image
	 */


	 if (DISPVID)  cvNamedWindow("Original");
	 if (DISPVID) cvNamedWindow("ToDLP");
	  cvNamedWindow("Boundary");
	 if (DISPVID) cvNamedWindow( "Thresholded");
	 if (DISPVID)  cvNamedWindow( "Contours", 1);
	 if (DISPVID) cvNamedWindow("Controls");
	 if (DISPVID) cvResizeWindow("Controls",300,400);



	cvCreateTrackbar("Threshold", "Controls", &(Params->BinThresh),255, on_trackbar);
	cvCreateTrackbar("Gauss=x*2+1","Controls", &(Params->GaussSize),5, on_trackbar);
	cvCreateTrackbar("ScalePx","Controls", &(Params->LengthScale),50,on_trackbar);
	cvCreateTrackbar("TemporalIQ","Controls",&(Params->TemporalOn),1, on_trackbar);
	cvCreateTrackbar("Proximity","Controls",&(Params->MaxLocationChange),100, on_trackbar);
	cvCreateTrackbar("NumSegments","Controls",&(Params->NumSegments),200, on_trackbar);



	/** SetUp Write Out to File **/
	CvFileStorage* fs=cvOpenFileStorage("data.yaml",Worm->MemStorage,CV_STORAGE_WRITE);
	cvStartWriteStruct(fs,"Frames",CV_NODE_SEQ,NULL);



	int i=0;
	while(1){

		if (i!=0) tempImg=cvQueryFrame( capture);
		if (tempImg==NULL) {
			printf("tempImg is NULL at frame %d.\n I assume this means we're done.",i);
			break;
		}
		i++;
		LoadWormColorOriginal(Worm,tempImg);
		on_trackbar(0);

		/** Write Out Data to File **/
		cvStartWriteStruct(fs,NULL,CV_NODE_MAP,NULL);
			cvWriteInt(fs,"FrameNumber",i);
			if (  (Worm->Segmented->Head->x >=0) && (Worm->Segmented->Head->y >= 0) ){

			cvStartWriteStruct(fs,"Head",CV_NODE_MAP,NULL);
			cvWriteInt(fs,"x",0);
				cvWriteInt(fs,"x",Worm->Segmented->Head->x);
				cvWriteInt(fs,"y",Worm->Segmented->Head->y);
			cvEndWriteStruct(fs);
			}
			cvStartWriteStruct(fs,"Tail",CV_NODE_MAP,NULL);
				cvWriteInt(fs,"x",Worm->Segmented->Tail->x);
				cvWriteInt(fs,"y",Worm->Segmented->Tail->y);
			cvEndWriteStruct(fs);


			cvWrite(fs,"BoundaryA",Worm->Segmented->LeftBound);
			cvWrite(fs,"BoundaryB",Worm->Segmented->RightBound);
			cvWrite(fs,"SegmentedCenterline",Worm->Segmented->Centerline);
		cvEndWriteStruct(fs);

		char c= cvWaitKey(1);
		if (c==27) break;
	}


	/** Finish writing this structure **/
	cvEndWriteStruct(fs);
	/** Close File Storage and Finish Writing Out to File **/
	cvReleaseFileStorage(&fs);

if (0){
	/*
	 * Time Test
	 *
	 */
	int NumLoops=1000;
	int i;
	printf("Ready...\n");
	cvWaitKey();
	printf("Start a Thousand Loops\n");
	for (i=0; i<NumLoops; i++){
	on_trackbar(0);
	if (i%100==0) printf(".");
	}
	printf("Finished!\n");
}


	cvWaitKey(0);
	DestroyWormAnalysisDataStruct(Worm);
	DestroyWormAnalysisParam(Params);



	return 0;
}


