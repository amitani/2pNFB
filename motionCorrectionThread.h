#ifndef MOTION_CORRECTION_THREAD
#define MOTION_CORRECTION_THREAD



#include <stdint.h> 
#include <vector>
#include <deque>
#include <fstream>
#include <memory>
using namespace std;

#include "shared_mmap.h"

#include "BilinearRegistrator.h"


struct activity_args_t{
	int32_t channel;
    int32_t n_roi;
	vector<double> roiMasks;
};

struct result_t{
	image_t source_image;
	
	double dislocation[2];
// 	std::vector<std::vector<int16_t>> shifted_image;
    image_t shifted_image;
    
	std::vector<double> raw_intensity;

};

class thread_params_t{

private:
	unsigned int to_stop;
	CRITICAL_SECTION  to_stop_critical_section;

	deque<shared_ptr<result_t>> list_p_result;
	CRITICAL_SECTION  cs_list_p_result;

	bool thread_params_t::decrease_to_stop();
	
	void thread_params_t::add_p_result(shared_ptr<result_t> p_result);

public:
	struct params{
		shared_ptr<BilinearRegistrator<int16_t, int64_t>> registrator;
		int align_channel;
		int margin;

		activity_args_t activity;
	};

	static const int history_length = 1000;
	
	shared_mmap* mmap;
	shared_mmap* mmap_result;
	shared_mmap* mmap_shifted;
	shared_mmap* mmap_average;
	shared_mmap* mmap_dislocation;
    int average_window;
	
	vector<string> warning_buffer;
	CRITICAL_SECTION  warning_buffer_critical_section;
	
	thread_params_t::thread_params_t();
	thread_params_t::~thread_params_t();
	
	void thread_params_t::increase_to_stop(unsigned int value = 1);
	
	std::shared_ptr<params> p_params; // should be thread-safe
	
	std::deque<std::shared_ptr<result_t>> thread_params_t::get_p_result();

	static unsigned __stdcall SecondThreadFunc( void* pArguments );

};



unsigned __stdcall updateAverage( void* pArguments );

#endif