#include "str_motion_model.h"

#include <cmath>
#include <libconfig.h++>

namespace str
{

	motion_model::motion_model(libconfig::Config &cfg, const odom& initial)
	{
		alpha1_ = cfg.lookup("motionModel.alpha1");		
		alpha2_ = cfg.lookup("motionModel.alpha2");		
		alpha3_ = cfg.lookup("motionModel.alpha3");		
		alpha4_ = cfg.lookup("motionModel.alpha4");		
		current_reading_=initial;
	}


	void motion_model::propagate_particles(particles& init_particles)
	{
		for (auto& curr_particle:init_particles)
		{
		//Calculate normally distributed perturbations
		double theta1_rad = theta1_rad_ - sample_from_gaussian(alpha1_*theta1_rad_ + alpha2_*translation_cm_);

		double translation_cm = translation_cm_ - sample_from_gaussian(alpha3_*translation_cm_ + alpha4_*(theta1_rad_+theta2_rad_));   

		double theta2_rad = theta2_rad_ - sample_from_gaussian(alpha1_*theta2_rad_ + alpha2_*translation_cm_);
			
		curr_particle.x_cm+= translation_cm * std::cos(curr_particle.theta_deg + theta1_rad);
		curr_particle.y_cm+= translation_cm * std::sin(curr_particle.theta_deg + theta1_rad);
		curr_particle.theta_deg+=  angle_radians_to_degree(theta1_rad+theta2_rad);
		}
	}

	void motion_model::update_odometry(const odom& reading)
	{

		theta1_rad_ = std::atan2((reading.y_cm-current_reading_.y_cm),(reading.x_cm-current_reading_.x_cm)) 
		                      - angle_degree_to_radians(current_reading_.theta_deg); 

		translation_cm_ = std::hypot((reading.y_cm-current_reading_.y_cm),(reading.x_cm-current_reading_.x_cm));

		theta2_rad_ = angle_degree_to_radians(reading.theta_deg - current_reading_.theta_deg);

	// Trim the calculated angles if they are not between [-PI,PI]
		if(!check_angle(theta1_rad_))
		{
			trim_angle_radians(theta1_rad_);
		}

		if(!check_angle(theta2_rad_))
		{
			trim_angle_radians(theta2_rad_);
		}

	
	//Update the final odometry value
		current_reading_ = reading;
	}

}//end namespace str