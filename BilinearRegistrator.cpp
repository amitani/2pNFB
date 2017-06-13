#include "BilinearRegistrator.h"
#include <vector>
#include <iostream>

int main(){
	std::vector<int> source_image(512*512,0);
	source_image[255*512+256]=10;
	ImagePointer<int> source;
	source.image=&source_image[0];
	source.size[0]=512;
	source.size[1]=512;
	
	int target_image[16] ={10,10,0,0,10,10,0,0,0,0,0,0,0,0,0,0};
	ImagePointer<int> target;
	target.image=target_image;
	target.size[0]=4;
	target.size[1]=4;

	std::cout<<"target"<<std::endl;
	
	double result[3];
	
	BilinearRegistrator<int, long long> br(target, 10,2);
	br.align(source,result);
	std::cout<<"result"<<result[0]<<','<<result[1]<<','<<result[2]<<std::endl;
	
	return 0;
}

