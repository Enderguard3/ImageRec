#pragma once
#include "CNN.h"

#include <string>
#include <opencv2/opencv.hpp>
#include <caffe/caffe.hpp>
#include <algorithm>
#include <iosfwd>
#include <memory>
#include <string>
#include <utility>
#include <vector>

class CaffeCNN : public  CNN
{
public:
	CaffeCNN();
	~CaffeCNN();
	void train();
	std::string getPictureInfo(const cv::Mat& image);
	void updateModel(const cv::Mat& picture, bool correctlyIdentified);

	void prepareData();


private :
	std::string imagesLoc;
	std::string lmdbLoc;
	std::string trainLmdbLoc;
	std::string caffeModelLoc;
	std::string imageMeanLoc;
	std::string solverLoc;
	std::string trainerLoc;
	void prepareImageMean();
	void transformImagesDB();
	void resizeImagesDB();



};

