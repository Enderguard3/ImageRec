#include "YoloGPU.h"
#include <unistd.h>
extern "C" {
#include <darknet.h>
}

YoloGPU::YoloGPU(float _prob,std::map<std::string,std::string> setting) {


    cfg = cfgbuff;
    fil = filbuff;
    wei = weibuff;
    dat = datbuff;
    lbl = lblbuff;
    std::string test1 = setting.at("GPUKitchen_dat");
    std::string test2 = setting.at("GPUKitchen_cfg");
    std::string test3 = setting.at("GPUKitchen_wei");
    std::string test4 = setting.at("GPUKitchen_lbl");
    std::string test5 = setting.at("GPUDefault_lbl");
    std::string test6 = setting.at("GPUDefault_dat");
    std::string test7 = setting.at("GPUDefault_cfg");
    std::string test8 = setting.at("GPUDefault_wei");
//std::cout<<setting["GPUDefault_lbl"];

#ifdef USE_KITCHEN
    strncpy(dat,setting.at("GPUKitchen_dat"),256);
    strncpy(cfg,setting.at("GPUKitchen_cfg"),256);
    strncpy(wei,setting.at("GPUKitchen_wei"),256);
    strncpy(lbl,setting.at("GPUKitchen_lbl"),256);
#else
    strncpy(lbl,test5.c_str(),256);
    strncpy(dat,test6.c_str(),256);
    strncpy(cfg,test7.c_str(),256);
    strncpy(wei,test8.c_str(),256);
#endif


    thresh = _prob;
    names = get_labels(lbl);


    net = load_network(cfg,wei, 0);

    set_batch_network((network*)net, 1);

    srand(2222222);
    nms =.45;
    //thresh = 0.2;

}

YoloGPU::~YoloGPU() {
    free_network((network*)net);

    //free_list(options);
}


inline std::vector<DetectedObject> YoloGPU::findObjects(cv::Mat color,cv::Mat depth) {

    image im = make_image (color.size().width,color.size().height,color.channels());//= load_image_color(fil,0,0);

    int nbClasses = 6;
#ifdef USE_KITCHEN
    nbClasses = 20;
#endif
    int h = color.size().height;
        int w = color.size().width;
        int c = 3;
        int step = color.step;
        int i, j, k;
        for(i = 0; i < h; ++i){
            for(k= 0; k < c; ++k){
                for(j = 0; j < w; ++j){
                    im.data[k*w*h + i*w + j] = color.at<unsigned char>(i * step + j*c + k)/255.;
                }
            }
        }

    for(i = 0; i < im.w*im.h; ++i){
        float swap = im.data[i];
        im.data[i] = im.data[i+im.w*im.h*2];
        im.data[i+im.w*im.h*2] = swap;
    }

    image sized = letterbox_image(im, ((network*)net)->w, ((network*)net)->h);

    float *X = sized.data;
    time=what_time_is_it_now();
    network_predict((network*)net, X);
    //printf(" Predicted in %f seconds.\n",  what_time_is_it_now()-time);
    int nboxes = 0;
    detection *dets = get_network_boxes((network*)net, im.w, im.h, thresh, 0.5, 0, 1, &nboxes);
    if (nms) do_nms_sort(dets, nboxes, nbClasses, nms);
    std::vector<DetectedObject> objects;


    for (i = 0; i < nboxes;i++) {
        bool correct = false;
        int pos = -1;
        for (int j = 0; j <nbClasses;j++) {
            if (dets[i].prob[j] > thresh) {
                correct = true;
                pos = j;
            }
        }
        if (correct) {
            cv::Rect obj(0,0,0,0);
            auto b = dets[i].bbox;
            int left  = (b.x-b.w/2.)*im.w;
            int right = (b.x+b.w/2.)*im.w;
            int top   = (b.y-b.h/2.)*im.h;
            int bot   = (b.y+b.h/2.)*im.h;
            std::string nom = names[pos];

            if(left < 0) left = 0;
            if(right > im.w-1) right = im.w-1;
            if(top < 0) top = 0;
            if(bot > im.h-1) bot = im.h-1;
            obj.x = left;
            obj.y = top;
            obj.width = right - left;
            obj.height = bot - top;

            cv::Rect2d central;

            central.x = obj.x + obj.width/4;
            central.y = obj.y + obj.height/4;
            central.width = obj.width/2;
            central.height = obj.height/2;
            if (nom != "hand")
				central = obj;
/*
            central.x = obj.x + obj.width/2 -3;
            central.y = obj.y + obj.height/2 - 3;
            central.width = 3;
            central.height = 3;
*/

			auto maxi = mean(depth)[0];
            //cv::Scalar m = mean(depth(obj), depth(obj) <= maxi + 1 );
            //cv::Scalar m = mean(depth(obj));
			cv::Mat thresh,mask;
			threshold(depth(obj), thresh, maxi, maxi + 1, cv::THRESH_TOZERO_INV);
			thresh.convertTo(mask,CV_8U);
            cv::Scalar m= mean(depth(obj),mask);

			/*if(nom == "green")
				std::cout << "distance vert " << m[0] << " " << m2[0]  << std::endl;
			if(nom == "hand")
				std::cout << "distance main " << m[0] << " " << m2[0] << std::endl;	
				//if(nom == "blue")
				//std::cout << "distance bleu " << m[0]<< std::endl;	
				*/
						
            int offset = pos*123457 % 10;
            float red = get_color(2,offset,10);
             float green = get_color(1,offset,10);
             float blue = get_color(0,offset,10);
              float rgb[3];
              rgb[0] = red;
              rgb[1] = green;
              rgb[2] = blue;
              bool tosave = true;
              for (auto it = objects.begin(); it != objects.end();++it) {\
				  if (nom != "hand" && ((it->getObjPos() & obj).area() >= 0.8 * std::min(obj.area(), it->getObjPos().area()))) {
					if (m[0] > it->getProb()) {
						objects.erase(it);
						break;
					}
					else {
						tosave = false;
					}
				}
			  }
            if(tosave)
				objects.emplace_back(DetectedObject(obj,nom,m[0],dets[i].prob[pos],red,green,blue));
        }
    }
    std::vector<bool> posi(objects.size());
    std::fill(posi.begin(),posi.end(),true);

    std::vector<DetectedObject> objs;
    for (int i = 0; i < objects.size();i++) {
        DetectedObject obj(objects[i]);
        if (posi[i]) {
            for (int j = i + 1; j < objects.size();j++) {
                if (obj.getObjName() != objects[j].getObjName())
                    continue;
                int ar = (obj.getObjPos() & objects[j].getObjPos()).area();
                if ((ar * 1.3 > obj.getObjPos().area() || ar * 1.3 > objects[j].getObjPos().area()) &&
                        obj.getObjName() == objects[j].getObjName()) {
                    posi[i] = false;
                    posi[j] = false;
                    obj.fusePosition(objects[j]);
                }
            }
            objs.push_back(obj);
        }

    }

    free_detections(dets, nboxes);
    free_image(im);
    free_image(sized);
    //free_layer(l);
    return objs;
}


float YoloGPU::get_color(int c, int x, int max)
{
    float ratio = ((float)x/max)*5;
    int i = floor(ratio);
    int j = ceil(ratio);
    ratio -= i;
    float r = (1-ratio) * colors[i][c] + ratio*colors[j][c];
    //printf("%f\n", r);
    return r;
}
