#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>

using namespace cv;
using namespace std;

///////////////  Document Scanner //////////////////////

Mat imgOriginal, imgGray, imgBlur, imgCanny, imgThre, imgDil, imgErode, imgWarp, imgCrop; 

//The Mat class of OpenCV library is used to 
//store the values of an image. It represents an n-dimensional array and
//is used to store image data of grayscale or color images

vector<Point> initialPoints, docPoints;
float w = 420, h = 596;    // size of paper 
//
Mat preProcessing(Mat img)
{
	cvtColor(img, imgGray, COLOR_BGR2GRAY);  // converts BGR img to gray scale image
	GaussianBlur(imgGray, imgBlur, Size(3, 3), 3, 0); // blurs an image using gaussian filter 
	Canny(imgBlur, imgCanny, 25, 75); //detects edges in an image 
	Mat kernel = getStructuringElement(MORPH_RECT, Size(3, 3)); // using this it creates a kernel which is used in dilation and erosion
	//in python we use numpy as kernel but here we use structuringelement

	dilate(imgCanny, imgDil, kernel); //after detecting the images , the edges are thin , so we can dilate the edges with this.
	//erode(imgDil, imgErode, kernel);
	return imgDil;
}
//
vector<Point> getContours(Mat image) {

	vector<vector<Point>> contours;  //list of different points eg : {{point(20,30),point(50,60)},{},{}}

	vector<Vec4i> hierarchy;        //vector having 4 integer inside of it .

	// Countours is an outline representing or bounding the shape or form of something.
	findContours(image, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);  
	//drawContours(img, contours, -1, Scalar(255, 0, 255), 2);    
	vector<vector<Point>> conPoly(contours.size()); // corner points should can never exceed size of contours
	vector<Rect> boundRect(contours.size());    /// stores the values of rectangle

	vector<Point> biggest;
	int maxArea = 0; 

	for (int i = 0; i < contours.size(); i++)
	{
		int area = contourArea(contours[i]); // function to calculate area 
		//cout << area << endl;

		string objectType;

		if (area > 1000)  // to filter out the objects having area of less than 1000
		{
			float peri = arcLength(contours[i], true);    //bounding box around the paper (perimeter)
			approxPolyDP(contours[i], conPoly[i], 0.02 * peri, true);  //calculate the no. of corner points 

			if (area > maxArea && conPoly[i].size() == 4) // to check if the area is biggest and has 4 points 
			{

				//drawContours(imgOriginal, conPoly, i, Scalar(255, 0, 255), 5);  ///draw points of corner points on paper //////
				biggest = { conPoly[i][0],conPoly[i][1] ,conPoly[i][2] ,conPoly[i][3] }; // replace the biggest corner points . 
				maxArea = area;  //updation of biggest area that has been found yet 
			}
			//drawContours(imgOriginal, conPoly, i, Scalar(255, 0, 255), 2);
			//rectangle(imgOriginal, boundRect[i].tl(), boundRect[i].br(), Scalar(0, 255, 0), 5);
		}
	}
	return biggest;
}
//
void drawPoints(vector<Point> points, Scalar color) // drawa point at the corners
{
	for (int i = 0; i < points.size(); i++)
	{
		circle(imgOriginal, points[i], 10, color, FILLED);
		putText(imgOriginal, to_string(i), points[i], FONT_HERSHEY_PLAIN, 4, color, 4);
	}
}
//
vector<Point> reorder(vector<Point> points) /// to make the conPoly points  always in the same order 
{
	vector<Point> newPoints; //newPoints to send back 
	vector<int> sumPoints, subPoints;   /// sumpoints{68,430,750,300} (23,45) (400,30) 

	for (int i = 0; i < 4; i++)
	{
		sumPoints.push_back(points[i].x + points[i].y);
		subPoints.push_back(points[i].x-points[i].y);
	}

	newPoints.push_back(points[min_element(sumPoints.begin(), sumPoints.end()) - sumPoints.begin()]);//0  
	newPoints.push_back(points[max_element(subPoints.begin(), subPoints.end()) - subPoints.begin()]);//1
	newPoints.push_back(points[min_element(subPoints.begin(), subPoints.end()) - subPoints.begin()]);//2
	newPoints.push_back(points[max_element(sumPoints.begin(), sumPoints.end()) - sumPoints.begin()]);//3

	return newPoints;
}
//
Mat getWarp(Mat img, vector<Point> points, float w, float h)
{
	Point2f src[4] = { points[0],points[1],points[2],points[3] }; //points from the image (source)
	Point2f dst[4] = { {0.0f,0.0f},{w,0.0f},{0.0f,h},{w,h} };     // destination coordinates  

	Mat matrix = getPerspectiveTransform(src, dst);   //in order to transform we need a matrix 
	warpPerspective(img, imgWarp, matrix, Point(w, h));  // point is the size of imgWarp 

	return imgWarp;
}

void main() {

	string path = "Resources/paper3.jpg";
	imgOriginal = imread(path);
	resize(imgOriginal, imgOriginal, Size(), 0.5, 0.5);  // resizing the image by 50% 

	//// Preprpcessing – Step 1
	imgThre = preProcessing(imgOriginal);

	//// Get Contours – Biggest – Step 2
	initialPoints = getContours(imgThre);    //passes dilated image from getcontours 
	//drawPoints(initialPoints, Scalar(0, 0, 255));
	docPoints = reorder(initialPoints);
	//drawPoints(docPoints, Scalar(0, 255, 0));
	//drawPoints(initialPoints, Scalar(0, 0, 255));


	//// Warp – Step 3
	imgWarp = getWarp(imgOriginal, docPoints, w, h); // warp displays the image as its taken from topview.

	////Crop – Step 4
	int cropVal = 5;                           
	Rect roi(cropVal,cropVal,w-(2*cropVal),h-(2*cropVal)); //roi(x,y,w,h) // cropping the image for the paper 
	imgCrop = imgWarp(roi);

	imshow("Image", imgOriginal);
	imshow("Dilated Image", imgThre);
	imshow("Warp Image", imgWarp);
	imshow("ImageCrop", imgCrop);
	waitKey(0);

}