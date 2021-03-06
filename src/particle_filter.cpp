// Class that contains the particle filter 
// Needs the map to be initialized and is updated every time an update comes from a log 

#include "particle_filter.h"


namespace str
{

particle_filter::particle_filter(libconfig::Config &cfg, 
			odom &initial_odom, 
			const map_type& map, 
			Grapher& grapher):grapher_(grapher)
{	
	map_ = map;

 	// grapher_=grapher;
	// grapher_=grapher;

	n_particles_ = cfg.lookup("particles.nParticles");

	// create motion model
	motion_model_=std::make_shared<motion_model>(motion_model(cfg,initial_odom));

	// Create Sensor Model
  sensor_params_.uniformParam = cfg.lookup("sensorModel.uniformParam");
  sensor_params_.decayScale =   cfg.lookup("sensorModel.decayScale");
  sensor_params_.decayRate =    cfg.lookup("sensorModel.decayRate");
  sensor_params_.maxParam =     cfg.lookup("sensorModel.maxParam");
  sensor_params_.rangeSTD =     cfg.lookup("sensorModel.rangeSTD");
  sensor_params_.gaussianGain = cfg.lookup("sensorModel.gaussianGain");

  // Create observation Model and build range cache
  observation_model_ = std::make_shared<observation_model>(observation_model(sensor_params_));
  observation_model_->forcePopulateRangeCache(map_);

  kidnapped_robot_factor_ = cfg.lookup("kidnapped_factor");

}

void particle_filter::filter_update_odom(odom& odometry_reading)
{
	//Update the motion model
	motion_model_->update_odometry(odometry_reading);
	motion_model_->propagate_particles(particle_set_);

}

void particle_filter::filter_update_laser(laser& laser_reading, int enableSensorPlotting)
{	
	//std::cout<<"\nLaser Update";
	particles new_particles;
	double sum_of_weights = 0.0;

  	// auto get_prob_for_particle_bind = std::bind(&observation_model::getProbForParticle,std::placeholders::_1,std::placeholders::_2,map_,grapher_,enableSensorPlotting);

	for (size_t p_idx=0; p_idx < particle_set_.size(); ++p_idx)
	{
		//Update weight of particle based on sensor model
		observation_model_->getProbForParticle(particle_set_[p_idx],laser_reading,map_,grapher_, enableSensorPlotting);
		sum_of_weights+= particle_set_[p_idx].weight;
	}

	// std::cout<<"\nOld: "<<sum_of_particles_<<" New: "<<sum_of_weights;

	if (sum_of_weights / sum_of_particles_ <kidnapped_robot_factor_)
	{	
		std::cout<<"\nGenerated Random number of particles again. Robot Kidnapped.";
		particle_set_.clear();
		this->generate_random_particles();
		sum_of_particles_=sum_of_weights;
		// std::cout<<"\nParticle Size: "<<particle_set_.size();
		sum_of_particles_=sum_of_weights;
	}
	else
	{
	// std::cout<<"\nSum of weights before: "<< sum_of_weights;

	normalize_weights(particle_set_);

	//Resample the particles based on their updated weights
	resample(new_particles);

	//Copy these new particles to your particle set
	particle_set_.clear();
	// particle_set_.copy(new_particles.begin(), new_particles.end());
	particle_set_=new_particles;	

	sum_of_particles_=sum_of_weights;
	}


	
}

void particle_filter::resample(particles& new_particles)
{
	//Make sure new particles are empty

	//std::cout<<"\nResampling "<<new_particles.size()<<" "<<particle_set_.size();
	new_particles.clear();	

	
	unsigned int num_draws = particle_set_.size();
	double num_draws_inv = 1.0/num_draws;

	// Generate a random number between 0 and num_draws_inv
	// double random_number = (std::rand()/RAND_MAX)*num_draws_inv;
	double random_number = sample_from_uniform(0,num_draws_inv);

	//std::cout<<"\n\nRandom: "<<random_number;
	//std::cout<<"\nDraws Inv: "<<num_draws_inv;

	//First Weight 
	double w  = particle_set_.front().weight;

	// Upper bound for resampling
	double upper_bound = 0;

	unsigned int i= 0;

	for (size_t particle_idx = 0 ; particle_idx < particle_set_.size(); ++particle_idx)
	{
		upper_bound = random_number + 1.*(particle_idx)/particle_set_.size();
		
		while(upper_bound>w && i<particle_set_.size()-1)
		{
			
			++i;
			
			w+=particle_set_.at(i).weight;
		}
		new_particles.push_back(particle_set_.at(i));
	}
}

void particle_filter::generate_random_particles()
{
	// Compute free space for inital particle locations
	std::vector<std::pair<int, int>> freeSpace;
	unsigned int width = map_.size_x*map_.resolution;
	unsigned int height = map_.size_y * map_.resolution;
	for (unsigned int i = 0; i < width; ++i)
	{
	  for (unsigned int j = 0; j < height; ++j)
	  {
	    if(map_.prob[i/map_.resolution][j/map_.resolution] == 0){
	        freeSpace.push_back(std::pair<int, int>(i,j));
	    }
	  }
	}

	// Initialize particle set
	particle_set_.clear();
	for (unsigned int i = 0; i < n_particles_; i++)
	{
	    std::pair<int, int> pt;
	    int r_pt = (std::rand() * freeSpace.size()) / RAND_MAX ;
	    pt = freeSpace[r_pt];
	    str::particle newParticle(pt.first, pt.second, r_pt);
	    particle_set_.push_back(newParticle);
	}
}

particle particle_filter::get_centroid()
{
	double xsum, ysum, thsum = 0;
	for(auto p = particle_set_.begin(); p != particle_set_.end(); p++)
	{
		xsum += p->x_cm;
		ysum += p->y_cm;
		thsum += p->theta_rad;
	}
	particle centroid(0,0,0);
	centroid.x_cm = xsum / particle_set_.size();
	centroid.y_cm = ysum / particle_set_.size();
	centroid.theta_rad = thsum / particle_set_.size();
	return centroid;
}

particle particle_filter::get_max()
{	
	size_t max_idx =0 ;
	double max_weight = std::numeric_limits<double>::min();
	for(size_t p_idx = 0 ; p_idx < particle_set_.size(); ++p_idx)
	{
		if (particle_set_[p_idx].weight>max_weight)
		{
			max_weight = particle_set_[p_idx].weight;
			max_idx = p_idx;
		}
	}
	return particle_set_[max_idx];
}

}//end namespace str