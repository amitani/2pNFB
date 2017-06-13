#ifndef BILINEAR_REGISTRATOR
#define BILINEAR_REGISTRATOR
#include<vector>

#include<limits>
#include<math.h>
#include<iostream>
#include<memory>
#include<algorithm>

#ifdef _MSC_VER
#include <float.h>  // for _isnan() on VC++
#define isnan(x) _isnan(x)  // VC++ uses _isnan() instead of isnan()
#endif

template <typename T>
struct ImagePointer{
	T* image;
	unsigned int size[2];
	std::shared_ptr<std::vector<T>> image_vector;
	
	ImagePointer();
	ImagePointer(unsigned int x, unsigned int y);
	~ImagePointer();

	void shift(ImagePointer<T> dist, double i0, double j0);
	ImagePointer<T> shift(double i0, double j0);

	void shrink(ImagePointer<T> dist, int factor);
	ImagePointer<T> shrink(int factor);

	void trim(ImagePointer<T> dist, int margin);
	ImagePointer<T> trim(int margin);
};


template <typename T>
ImagePointer<T>::ImagePointer(){};

template <typename T>
ImagePointer<T>::ImagePointer(unsigned int x, unsigned int y){
	size[0] = x; size[1] = y;
	if (x*y > 0){
		image_vector = std::shared_ptr<std::vector<T>>(new std::vector<T>(size[0] * size[1]));
		image = &((*image_vector)[0]);
	}
};

template <typename T>
ImagePointer<T>::~ImagePointer(){
	//std::cout<<"destructor"<<std::endl;
	//if(image_vector)
	// std::cout<<image_vector.use_count()<<std::endl;
};


template <typename T>
void ImagePointer<T>::shift(ImagePointer<T> dist, double i0, double j0){
	if (size[0] != dist.size[0] || size[1] != dist.size[1]){
		std::cout << "[ImagePointer::shift]size doesn't match" << std::endl;
		return;
	}

	int i0_floor, j0_floor;
	double i0_frac, j0_frac;
	i0_floor = floor(i0); i0_frac = i0 - i0_floor;
	j0_floor = floor(j0); j0_frac = j0 - j0_floor;

	
	int j_min = std::max(0, -j0_floor);
	int j_max = std::min(size[1], size[1] - j0_floor - 1);
	int i_min = std::max(0, -i0_floor);
	int i_max = std::min(size[0], size[0] - i0_floor - 1);

	for (int j = j_min; j < j_max;j++){
		for (int i = i_min; i < i_max; i++){
			dist.image[j*dist.size[0] + i] =
				image[(j0_floor + j)*size[0] + i0_floor + i] * (1 - i0_frac)*(1 - j0_frac)
				+ image[(j0_floor + j)*size[0] + i0_floor + i + 1] * i0_frac*(1 - j0_frac)
				+ image[(j0_floor + j + 1)*size[0] + i0_floor + i] * (1 - i0_frac)*j0_frac
				+ image[(j0_floor + j + 1)*size[0] + i0_floor + i + 1] * i0_frac*j0_frac;
		}
	}

	/*
	if (j_max < size[0] && j0_frac == 0){
		int j = j_max;
		for (int i = i_min; i < i_max; i++){
			dist.image[j*dist.size[0] + i] =
				image[(j0_floor + j)*size[0] + i0_floor + i] * (1 - i0_frac)*(1 - j0_frac)
				+ image[(j0_floor + j)*size[0] + i0_floor + i + 1] * i0_frac*(1 - j0_frac);
		}
	}
	if (i_max < size[1] && i0_frac == 0){
		int i = i_max;
		for (int j = j_min; j< j_max; j++ ){
			dist.image[j*dist.size[0] + i] =
				image[(j0_floor + j)*size[0] + i0_floor + i] * (1 - i0_frac)*(1 - j0_frac)
				+ image[(j0_floor + j + 1)*size[0] + i0_floor + i] * (1 - i0_frac)*j0_frac;
		}
	}

	if (i_max < size[1] && i0_frac == 0 && j_max < size[0] && j0_frac == 0){
		int i = i_max;
		int j = j_max;
		dist.image[j*dist.size[0] + i] = image[(j0_floor + j)*size[0] + i0_floor + i] * (1 - i0_frac)*(1 - j0_frac);
	}
	*/

	/*
	for (int j = 0; j < size[0]; j++){
		for (int i = 0; i < size[1]; i++){
			dist.image[j*dist.size[0] + i] = (j0_floor + j < 0 || (j0_frac>0 && j0_floor + j + 1 >= size[0]) || i0_floor + i < 0 || (i0_frac>0 && i0_floor + i + 1 >= size[1])) ? 0 :
				image[(j0_floor + j)*size[0] + i0_floor + i] * (1 - i0_frac)*(1 - j0_frac)
				+ (i0_frac == 0 ? 0 : image[(j0_floor + j)*size[0] + i0_floor + i + 1] * i0_frac*(1 - j0_frac))
				+ (j0_frac == 0 ? 0 : image[(j0_floor + j + 1)*size[0] + i0_floor + i] * (1 - i0_frac)*j0_frac)
				+ (i0_frac*j0_frac == 0 ? 0 : image[(j0_floor + j + 1)*size[0] + i0_floor + i + 1] * i0_frac*j0_frac);
		}
	}
	*/
	return;
};
template <typename T>
ImagePointer<T> ImagePointer<T>::shift(double i0, double j0){
	ImagePointer<T> r(size[0], size[1]);
	shift(r, i0, j0);
	return r;
}

template <typename T>
void ImagePointer<T>::shrink(ImagePointer<T> dist, int factor){
	//std::cout<<shrink<<std::endl;
	unsigned int dist_size[2];
	for (int i = 0; i<2; i++) dist_size[i] = size[i] / factor;

	if (dist_size[0] != dist.size[0] || dist_size[1] != dist.size[1]){
		std::cout << "[ImagePointer::shrink]size doesn't match" << std::endl;
		return;
	}

	for (int j = 0; j<dist.size[1]; j++){
		for (int i = 0; i<dist.size[0]; i++){
			double tmp = 0;
			for (int jj = 0; jj<factor; jj++){
				for (int ii = 0; ii<factor; ii++){
					//std::cout<<'A'<<j<<i<<j*size[0]+i<<std::endl;
					tmp += image[(j*factor + jj)*size[0] + i*factor + ii];
				}
			}
			dist.image[j*dist.size[0] + i] = tmp/factor/factor;
		}
	}
	return;
}
template <typename T>
ImagePointer<T> ImagePointer<T>::shrink(int factor){
	unsigned int dist_size[2];
	for (int i = 0; i<2; i++) dist_size[i] = size[i] / factor;
	ImagePointer<T> r(dist_size[0], dist_size[1]);
	shrink(r, factor);
	return r;
}


template <typename T>
void ImagePointer<T>::trim(ImagePointer<T> dist, int margin){
	//std::cout<<trim<<std::endl;
	unsigned int dist_size[2];
	for (int i = 0; i < 2; i++) dist_size[i] = std::max<int>(0, size[i] - 2 * margin);

	if (dist_size[0] != dist.size[0] || dist_size[1] != dist.size[1]){
		std::cout << "[ImagePointer::trim]size doesn't match" << std::endl;
		return;
	}

	for (int j = 0; j<dist.size[1]; j++){
		for (int i = 0; i<dist.size[0]; i++){
			dist.image[j*dist.size[0] + i] = image[(j + margin)*size[0] + i + margin];
		}
	}
	return;
}
template <typename T>
ImagePointer<T> ImagePointer<T>::trim(int margin){
	unsigned int dist_size[2];
	for (int i = 0; i < 2; i++) dist_size[i] = std::max<int>(0,size[i] - 2 * margin);
	ImagePointer<T> r(dist_size[0], dist_size[1]);
	trim(r, margin);
	return r;
}







//////////////////////////////////////////////////////////////////////////////////////////


template <typename T_target,typename T_internal>
class BilinearRegistrator{
private:
	ImagePointer<T_target> target;
	T_internal target_sum;
	T_internal target_square_sum;
	
	unsigned int depth;
	unsigned int factor;
	BilinearRegistrator* child;

	bool subpixel;
	bool nongreedy;
    
    unsigned int minimum_depth;
	
	template <typename T_source>
	class CorrelationCalculator{
	private:
		std::vector<std::vector<double>> computed_correlation;
		ImagePointer<T_target> target;
		ImagePointer<T_source> source;
		T_internal target_sum;
		T_internal target_square_sum;
		int max_xy[2];
	public:
		CorrelationCalculator(ImagePointer<T_target> target,ImagePointer<T_source> source, T_internal target_sum, T_internal target_square_sum);
		double get_correlation(int xy[]);
	};
	
public:
	BilinearRegistrator(ImagePointer<T_target> target,unsigned int depth = 3, unsigned int factor = 2,
            bool subpixel = true, bool nongreedy = true, unsigned int minimum_depth = 1);
	~BilinearRegistrator();
	template <typename T_source>
	void align(ImagePointer<T_source>& source, double result[], int initial_xy[] = NULL);
};



template <typename T_target,typename T_internal>
BilinearRegistrator<T_target, T_internal>::BilinearRegistrator(ImagePointer<T_target> target, unsigned int depth,
        unsigned int factor, bool subpixel, bool nongreedy, unsigned int minimum_depth)
 :target(target),depth(depth),child(NULL),factor(factor),subpixel(subpixel),minimum_depth(minimum_depth){
	//std::cout<<"constructor_depth"<<depth<<std::endl;
	target_sum = 0;
	target_square_sum = 0;
	for(int j = 0; j < target.size[1]; j++){
		for(int i = 0; i < target.size[0]; i++){
			int target_pixel_index = j * target.size[0]+i;
			T_internal target_pixel_value=target.image[target_pixel_index];
			target_sum += target_pixel_value;
			target_square_sum += target_pixel_value * target_pixel_value;
		}
	}
	if(depth>0 && target.size[0]/2>=factor && target.size[1]/2>=factor){
		ImagePointer<T_target> shrunk_target = target.shrink(factor);
		child = new BilinearRegistrator<T_target, T_internal>(shrunk_target, depth - 1, factor, true, nongreedy, minimum_depth - 1);
		this->nongreedy = false;
	}
	else{
		this->depth = 0;
		this->nongreedy = nongreedy;
	}
	//std::cout<<"constructor_depth_f"<<this->depth<<std::endl;
};
template <typename T_target,typename T_internal>
BilinearRegistrator<T_target,T_internal>::~BilinearRegistrator(){
	if(child) delete(child);
};



template <typename T_target,typename T_internal>
template <typename T_source>
void BilinearRegistrator<T_target,T_internal>
::align(ImagePointer<T_source>& source, double result[], int initial_xy[]){
	// std::cout<<"align_depth"<<depth<<std::endl;
	int current_xy[2];
	int max_xy[2];
	for (int i = 0; i < 2; i++){
		current_xy[i] = initial_xy ? initial_xy[i] : (source.size[i] - this->target.size[i]) / 2;
		max_xy[i] = (source.size[i] - this->target.size[i]);
	}

	if (depth > 0){
		double shrunk_result[3];
		int shrunk_initial_xy[2];
		for (int i = 0; i < 2; i++) shrunk_initial_xy[i] = current_xy[i] / factor;
		ImagePointer<T_source> shrunk_source = source.shrink(factor);
		child->align(shrunk_source, &(shrunk_result[0]), &(shrunk_initial_xy[0]));
		//std::cout<<"shrunk_result"<<shrunk_result[0]<<shrunk_result[1]<<shrunk_result[2]<<std::endl;
        if(minimum_depth>0){
            for (int i = 0; i < 2; i++) result[i] = shrunk_result[i] * factor;
            result[2] = shrunk_result[2];
            return;
        }else{
             for (int i = 0; i < 2; i++) current_xy[i] = shrunk_result[i] * factor;
        }
	}

	if (max_xy[0] <= 0 || max_xy[1] <= 0) return; // this should not happen
	// std::cout<<"AAA";
	CorrelationCalculator<T_source> correlation_calculator(target, source, target_sum, target_square_sum);

	if (nongreedy){
		int xy[2];
		double max_corr = -3;
		for (xy[0] = 0; xy[0] <= max_xy[0]; xy[0]++){
			for (xy[1] = 0; xy[1] <= max_xy[1]; xy[1]++){
				double corr = correlation_calculator.get_correlation(xy);
				if (corr > max_corr){
					max_corr = corr;
					for (int i = 0; i < 2; i++) current_xy[i] = xy[i];
				}
			}
		}
	}else{
		static const int dxy[9][2] = { { 0, 0 }, { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 }, { -1, -1 }, { 1, -1 }, { -1, 1 }, { 1, 1 } };
		bool is_max_reached = false;
		double next_correlation[sizeof(dxy) / sizeof(dxy[0])];
		while (!is_max_reached){
			int argmax = 0;
			double max = correlation_calculator.get_correlation(current_xy);
			next_correlation[0] = max;
			//std::cout<<max<<std::endl;
			for (int i = 1; i < sizeof(dxy) / sizeof(dxy[0]); i++){
				int next_xy[2];
				for (int j = 0; j<2; j++) next_xy[j] = current_xy[j] + dxy[i][j];
				// std::cout<<"nxy"<<next_xy[0]<<next_xy[1]<<std::endl;
				next_correlation[i] = correlation_calculator.get_correlation(next_xy);
				// std::cout<<"nc"<<next_correlation<<std::endl;
				if (next_correlation[i]>max){
					argmax = i;
					max = next_correlation[i];
				}
			}
			// std::cout<<"aargmax"<<argmax<<std::endl;
			if (argmax == 0)
				is_max_reached = true;
			else
				for (int j = 0; j < 2; j++) current_xy[j] = current_xy[j] + dxy[argmax][j];
		}
	}
	// std::cout<<'F';
	
	bool at_edge = (current_xy[0] == 0 || current_xy[0] == max_xy[0] || current_xy[1] == 0 || current_xy[1] == max_xy[1]);
	
	for (int j = 0; j<2; j++) result[j] = current_xy[j];
	result[2] = correlation_calculator.get_correlation(current_xy);
	if(subpixel&&!at_edge){
		static const int dxy[5][2] = { { 0, 0 }, { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 }};
		double next_correlation[sizeof(dxy) / sizeof(dxy[0])];
		for (int i = 0; i < sizeof(dxy) / sizeof(dxy[0]); i++){
			int next_xy[2];
			for (int j = 0; j<2; j++) next_xy[j] = current_xy[j] + dxy[i][j];
			next_correlation[i] = correlation_calculator.get_correlation(next_xy);
		}
		double p[3];
		p[0] = next_correlation[0]-(next_correlation[1]+next_correlation[2]+next_correlation[3]+next_correlation[4])/4;
		if(p[0]>0){
			p[1] = (next_correlation[2]-next_correlation[1])/2;
			p[2] = (next_correlation[4]-next_correlation[3])/2;
			result[0] += p[1]/(2*p[0]);
			result[1] += p[2]/(2*p[0]);
			result[2] += (p[1]*p[1]+p[2]*p[2])/(4*p[0]);
			if (result[2] > 1) result[2] = 1;
		}
	}
	
	return;
}



template <typename T_target,typename T_internal>
template <typename T_source>
BilinearRegistrator<T_target,T_internal>
 ::CorrelationCalculator<T_source>
  ::CorrelationCalculator(ImagePointer<T_target> target,ImagePointer<T_source> source, T_internal target_sum, T_internal target_square_sum)
:target(target),
 source(source),
 target_sum(target_sum),
 target_square_sum(target_square_sum)
 {
	// std::cout<<'B';
	for(int i=0;i<2;i++)max_xy[i]=source.size[i]-target.size[i];
	// std::cout<<'C'<<max_xy[0]<<max_xy[1];
	computed_correlation = std::vector<std::vector<double>>(max_xy[0]+1,
		std::vector<double>(max_xy[1]+1,std::numeric_limits<double>::quiet_NaN()));
	// std::cout<<'D';
}

template <typename T_target,typename T_internal>
template <typename T_source>
double BilinearRegistrator<T_target,T_internal>
 ::CorrelationCalculator<T_source>::get_correlation(int xy[]){
	// std::cout<<xy[0]<<xy[1]<<std::endl;
	// std::cout<<max_xy[0]<<max_xy[1]<<std::endl;
	if(xy[0]<0 || xy[1] < 0 || xy[0] > max_xy[0] || xy[1] > max_xy[1]) return -2;
	// std::cout<<computed_correlation.size()<<computed_correlation[0].size()<<std::endl;
	if(!isnan(computed_correlation[xy[0]][xy[1]])) return computed_correlation[xy[0]][xy[1]];
	
	
	
	T_internal dot_product = 0;
	T_internal source_sum = 0;
	T_internal source_square_sum = 0;
	
	for(int j = 0; j < target.size[1]; j++){
 		for(int i = 0; i < target.size[0]; i++){
			int target_pixel_index = j * target.size[0]+i;
			int source_pixel_index = (j + xy[1]) * source.size[0]+ i+xy[0];
			// std::cout<<j<< i <<  target_pixel_index << source_pixel_index <<std::endl;
			T_internal target_pixel_value=target.image[target_pixel_index];
			T_internal source_pixel_value=source.image[source_pixel_index];
			// std::cout<<target_pixel_value<< source_pixel_value <<std::endl;
			
			dot_product += target_pixel_value * source_pixel_value;
			source_sum += source_pixel_value;
			source_square_sum += source_pixel_value * source_pixel_value;
		}
	}
	
	double n = target.size[0]*target.size[1];
	double ss = source_square_sum - source_sum * source_sum/n;
	double st = target_square_sum - target_sum * target_sum/n;
	if(ss==0 && st==0)
		computed_correlation[xy[0]][xy[1]] = 1;
	else if (ss==0 || st==0)
		computed_correlation[xy[0]][xy[1]] = -1;
	else
		computed_correlation[xy[0]][xy[1]] = (dot_product - source_sum * target_sum/n )
						 /sqrt(ss)/sqrt(st);
	
	// std::cout << dot_product << n <<  source_sum << target_sum << source_square_sum << target_square_sum <<std::endl;
			
	return computed_correlation[xy[0]][xy[1]];
}



#endif
