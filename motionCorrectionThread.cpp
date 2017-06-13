#include "motionCorrectionThread.h"

#include <memory>
#include <process.h> //used for thread

thread_params_t::thread_params_t(){
	to_stop = 0;

	mmap= NULL;
	mmap_result = NULL;
	
	InitializeCriticalSection(&to_stop_critical_section);
	InitializeCriticalSection(&cs_list_p_result);
	
    average_window = 200;
    
	return;
		
}

thread_params_t::~thread_params_t(){
	delete mmap;
	delete mmap_result;
    delete mmap_shifted;
    delete mmap_average;
    delete mmap_dislocation;
    
	DeleteCriticalSection(&to_stop_critical_section);
	DeleteCriticalSection(&cs_list_p_result);
	
	return;
}

bool thread_params_t::decrease_to_stop(){
		bool to_stop_local;
		
		EnterCriticalSection(&to_stop_critical_section);
		if (to_stop > 0){
			to_stop_local = true;
			to_stop--;
		}
		else
			to_stop_local = false;
		
		LeaveCriticalSection(&to_stop_critical_section);

		return to_stop_local;
}


void thread_params_t::increase_to_stop(unsigned int value){
		EnterCriticalSection(&to_stop_critical_section);
		to_stop += value;
		LeaveCriticalSection(&to_stop_critical_section);

		return;
}

void thread_params_t::add_p_result(std::shared_ptr<result_t> p_result){		
		EnterCriticalSection(&cs_list_p_result);
			list_p_result.insert(list_p_result.begin(),p_result);
			if(list_p_result.size() > history_length){
				list_p_result.resize(history_length);
			}
		LeaveCriticalSection(&cs_list_p_result);
		return;
}

std::deque<std::shared_ptr<result_t>> thread_params_t::get_p_result(){		
		EnterCriticalSection(&cs_list_p_result);
			std::deque<std::shared_ptr<result_t>> copied_list_p_result = list_p_result;
		LeaveCriticalSection(&cs_list_p_result);
		return copied_list_p_result;
}

unsigned __stdcall thread_params_t::SecondThreadFunc( void* pArguments )
{
	thread_params_t* args = (thread_params_t*)pArguments;
    
	auto p_result = make_shared<result_t>();

	while(1){;
        if (args->decrease_to_stop()) break;

		// watch mmap
		if(!args->mmap->copy_image_data_to(&p_result->source_image,true)) continue;
		image_header_t* p_header = &p_result->source_image.header;
        if(p_header->channel == 0) continue;
		
		std::shared_ptr<params> p_params = args->p_params;

		// image alignment
		ImagePointer<int16_t> source;
		source.image = reinterpret_cast<int16_t*>(p_result->source_image.p_at(p_params->align_channel,0,0));
        if(!source.image) continue;
		source.size[0] = p_header->height;
		source.size[1] = p_header->width;
		double result[3];
		p_params->registrator->align(source, &result[0]);

		int margin = p_params->margin;
		for (int i = 0; i < 2; i++)  p_result->dislocation[i] = - result[i] + margin;

		// shift source image
        p_result->shifted_image.header = *p_header;
		p_result->shifted_image.resize(p_header->channel,p_header->height,p_header->width,p_header->opencv_data_type);
		for(unsigned k=0;k<p_header->channel;k++){
			ImagePointer<int16_t> dist;
			dist.size[0] = p_header->height;
			dist.size[1] = p_header->width;
			dist.image = reinterpret_cast<int16_t*>(p_result->shifted_image.p_at(k,0,0));
            if(!dist.image)continue;
            source.image = reinterpret_cast<int16_t*>(p_result->source_image.p_at(k,0,0));
            if(!source.image)continue;
			source.shift(dist, -p_result->dislocation[0], -p_result->dislocation[1]);
		}

		// readout
        p_result->raw_intensity.assign(p_params->activity.n_roi,0);
		for(unsigned j=0;j<p_params->activity.roiMasks.size()/3;j++){
            int i_pixel = -1 + p_params->activity.roiMasks[j];
			int i_roi = -1 + p_params->activity.roiMasks[j + 1 * p_params->activity.roiMasks.size() / 3];
			double weight = p_params->activity.roiMasks[j + 2 * p_params->activity.roiMasks.size() / 3];
            char* p = p_result->shifted_image.p_at(p_params->activity.channel,i_pixel);
			if (p && i_roi < p_result->raw_intensity.size())
				p_result->raw_intensity[i_roi] += weight * *(reinterpret_cast<int16_t*>(p));
			else
				continue;
		}

		// store results in mmap
		image_header_t result_header = p_result->source_image.header;
		result_header.width = 1;
		result_header.height = p_result->raw_intensity.size();
		result_header.channel = 1;
		result_header.opencv_data_type = 6; // double
		
		args->mmap_result->copy_image_data_from(
			result_header,
			reinterpret_cast<char*> (&(p_result->raw_intensity[0])),
			p_result->raw_intensity.size()*sizeof(p_result->raw_intensity[0]));
		
		// store shifted in mmap
		args->mmap_shifted->copy_image_data_from(p_result->shifted_image);
		
		// store in the list
		args->add_p_result(p_result);
        
        // reserve the same amount of memory for next
		p_result = make_shared<result_t>();
        p_result->source_image.data.reserve(p_header->height*p_header->width*p_header->channel*sizeof(int16_t));
		p_result->shifted_image.resize(p_header->channel,p_header->height, p_header->width,p_header->opencv_data_type);
		p_result->raw_intensity.resize(p_params->activity.n_roi,0);

		// update average once in a while
        if(0 == p_header->frame_tag % args->average_window)
			_beginthreadex(NULL, 0, &updateAverage, args, 0, NULL);
                
	}
	
	return 0;
} 


unsigned __stdcall updateAverage( void* pArguments )
{
	thread_params_t* thread_params = (thread_params_t*)pArguments;
    
    std::deque<std::shared_ptr<result_t>> list_p_result=thread_params->get_p_result();

    if(list_p_result.size()==0){
        return 1;
    }
    
    int average_window = list_p_result.size() >= thread_params->average_window ?
        thread_params->average_window : list_p_result.size();
    
    for(int i=1;i<average_window;i++){
        if(list_p_result[0]->shifted_image.header.height != list_p_result[i]->shifted_image.header.height
            ||list_p_result[0]->shifted_image.header.width != list_p_result[i]->shifted_image.header.width
            ||list_p_result[0]->shifted_image.header.channel != list_p_result[i]->shifted_image.header.channel){
            average_window = i;
            break;
        }
    }
        
    int image_size = list_p_result[0]->source_image.header.height 
        * list_p_result[0]->source_image.header.width;
    
    std::vector<int16_t> average(image_size * list_p_result[0]->shifted_image.header.channel);
    double dislocation[2]={0,0};
    for(int i=0;i<average_window;i++){
        for(int channel = 0;channel<list_p_result[0]->shifted_image.header.channel;channel++){
            for(int pixel = 0;pixel< image_size;pixel++){
                average[channel*image_size+pixel]
                    += double(*((int16_t*)list_p_result[i]->shifted_image.p_at(channel,pixel)))/double(average_window);
            }
        }
        for(int i=0;i<2;i++){
            dislocation[i] += list_p_result[i]->dislocation[i]/double(average_window);
        }
    }
    
    thread_params->mmap_average->copy_image_data_from(
        list_p_result[0]->source_image.header,
        reinterpret_cast<char*> (&(average[0])),
        average.size()*sizeof(average[0]));
    
    
    image_header_t result_header = list_p_result[0]->source_image.header;
    result_header.width = 1;
    result_header.height = 2;
    result_header.channel = 1;
    result_header.opencv_data_type = 6; // double

    thread_params->mmap_dislocation->copy_image_data_from(
        result_header,
        reinterpret_cast<char*>(dislocation), 2*sizeof(double));

    return 0;
}

