#include <vn_200/vn200_node.hpp>
#include <vn_200/vn200.h>
#include <vn_200/vectornav.h>

using namespace vectornav;

namespace vectornav{
    
    VN200::VN200() {
        /* Try to connect to the VN200 IMU. */
        errorCode = vn200_connect(
            &vn200,
            COM_PORT,
            BAUD_RATE);

        /* Make sure the user has permission to use the COM port. */
        if (errorCode == VNERR_PERMISSION_DENIED) {
            ROS_FATAL("Current user does not have permission to open the COM port.\n");
            ROS_FATAL("Try running 'sudo chmod 777 /dev/ttyUSB0'.\n");
            exit(0);
        }

        /* Error encountered while connecting. Please reconnect. */
        if (errorCode != VNERR_NO_ERROR){
            ROS_FATAL("Error encountered when trying to connect to the sensor.\n");
            exit(0);
        }

        /* Set cumulative initially to zero. */
        avgAcc.c0 = avgAcc.c1 = avgAcc.c2 = 0;
        avgYpr.yaw = avgYpr.pitch = avgYpr.roll = 0;

        /* Matrix C default to be Identity Matrix and vector B to be zero vector. */
        C.c00 = C.c11 = C.c22 = 1;
        C.c01 = C.c02 = C.c10 = C.c12 = C.c20 = C.c21 = 0;
        B.c0 = B.c1 = B.c2 = 0;

        /* Calculate averages at initial condition of rest. */
        for(i = 1; i <= iterAvg; i++){

            errorCode= vn200_getAcceleration(
                &vn200,
                &acceleration);

            errorCode= vn200_getYawPitchRoll(
                &vn200,
                &ypr);
        
            /* Calculate cumulatives. */
            avgYpr.yaw += ypr.yaw;
            avgYpr.pitch += ypr.pitch;
            avgYpr.roll += ypr.roll;
        
            avgAcc.c0 += acceleration.c0;
            avgAcc.c1 += acceleration.c1;
            avgAcc.c2 += acceleration.c2;

            usleep(50000);
        }

        /* Calculate averages. */
        avgAcc.c0 /=iterAvg;
        avgAcc.c1 /=iterAvg;
        avgAcc.c2 /=iterAvg;

        avgYpr.yaw /=iterAvg;
        avgYpr.pitch /=iterAvg;
        avgYpr.roll /=iterAvg;

        sequence_id = 0;
    }

    VN200::~VN200(){
    
        /* Discconect vn200 device. */
        errorCode = vn200_disconnect(&vn200);
        if (errorCode != VNERR_NO_ERROR){
            ROS_FATAL("Error encountered when trying to disconnect from the sensor.\n");
            exit(0);
        }
    }

	void GetInsSolution()
	{
		VN_ERROR_CODE errorCode;
    		double gpsTime;
    		unsigned short gpsWeek, status;
    		VnVector3 ypr, latitudeLognitudeAltitude, nedVelocity;
    		float attitudeUncertainty, positionUncertainty, velocityUncertainty;
    		Vn200 vn200;
    		int i;
		
		vn200_getInsSolutionLla(
		    &vn200,
		    &gpsTime,
		    &gpsWeek,
		    &status,
		    &ypr,
		    &latitudeLognitudeAltitude,
		    &nedVelocity,
		    &attitudeUncertainty,
		    &positionUncertainty,
		    &velocityUncertainty);
		printf("INS Solution:\n"
		    "  GPS Time:               %f\n"
		    "  GPS Week:               %u\n"
		    "  INS Status:             %.4X\n"
		    "  YPR.Yaw:                %+#7.2f\n"
		    "  YPR.Pitch:              %+#7.2f\n"
		    "  YPR.Roll:               %+#7.2f\n"
		    "  LLA.Latitude:           %+#7.2f\n"
		    "  LLA.Longitude:          %+#7.2f\n"
		    "  LLA.Altitude:           %+#7.2f\n"
		    "  Velocity.North:         %+#7.2f\n"
		    "  Velocity.East:          %+#7.2f\n"
		    "  Velocity.Down:          %+#7.2f\n"
		    "  Attitude Uncertainty:   %+#7.2f\n"
		    "  Position Uncertainty:   %+#7.2f\n"
		    "  Velocity Uncertainty:   %+#7.2f\n",
		    gpsTime,
		    gpsWeek,
		    status,
		    ypr.c0,
		    ypr.c1,
		    ypr.c2,
		    latitudeLognitudeAltitude.c0,
		    latitudeLognitudeAltitude.c1,
		    latitudeLognitudeAltitude.c2,
		    nedVelocity.c0,
		    nedVelocity.c1,
		    nedVelocity.c2,
		    attitudeUncertainty,
		    positionUncertainty,
		    velocityUncertainty);
		printf("\n\n");
		sleep(1);
	
	}

sensor_msgs::NavSatFix VN200::getGPSData(){
	unsigned short gpsWeek;
	unsigned char gpsFix, numberOfSatellites;
	float speedAccuracy1, timeAccuracy1;
	double gpsTime;
	VnVector3 latitudeLognitudeAltitude, nedVelocity, positionAccuracy1;
	/*unsigned short gpsWeek;
	unsigned char gpsFix, numberOfSatellites;
	float speedAccuracy1, timeAccuracy1, attitudeUncertainty, positionUncertainty, velocityUncertainty, temperature, pressure;
	double gpsTime, latitude, longitude, altitude;
	VnVector3 magnetic, acceleration1, angularRate, ypr1, latitudeLognitudeAltitude, nedVelocity, positionAccuracy1;
	*/
	vn200_getGpsSolutionLla(&vn200, 
	&gpsTime, 
	&gpsWeek, 
	&gpsFix, 
	&numberOfSatellites, 
	&latitudeLognitudeAltitude, 
	&nedVelocity, 
	&positionAccuracy1, 
	&speedAccuracy1, 	
	&timeAccuracy1);
ROS_INFO("Triangulating from %d satellites\n", numberOfSatellites);

printf("INS Solution:\n"
    	"  GPS Time:               %f\n"
    	"  GPS Week:               %u\n"
    	"  LLA.Latitude:           %+#7.2f\n"
    	"  LLA.Longitude:          %+#7.2f\n"
    	"  LLA.Altitude:           %+#7.2f\n",
    	gpsTime,
    	gpsWeek,
    	latitudeLognitudeAltitude.c0,
    	latitudeLognitudeAltitude.c1,
    	latitudeLognitudeAltitude.c2);
printf("\n\n");

sensor_msgs::NavSatFix gps_msg;

gps_msg.header.seq = sequence_id_gps++;
gps_msg.header.stamp = ros::Time::now();
gps_msg.header.frame_id = "gps/fix";
gps_msg.latitude=latitudeLognitudeAltitude.c0;
gps_msg.longitude=latitudeLognitudeAltitude.c1;
gps_msg.altitude=latitudeLognitudeAltitude.c2;

sleep(1);
return gps_msg;
}

    sensor_msgs::Imu VN200::getIMUData(){
	
        /* Start calibrated sensor measurement. */
        errorCode= vn200_getAcceleration(
            &vn200,
            &acceleration);     

        errorCode = vn200_getYawPitchRoll(
            &vn200,
            &ypr);
        
        /* Set offset vector B to be the averages. */
        B.c0 = avgAcc.c0;
        B.c1 = avgAcc.c1;
        B.c2 = avgAcc.c2;

        /* Set vector B in the acceleration compensation register. */
        vn200_setAccelerationCompensation(
            &vn200,
            C,
            B,
            true);

        /* Get calibrated accelerometer measurements. */
        errorCode = vn200_getQuaternionMagneticAccelerationAngularRate(
            &vn200,
            &attitude,
            &magnetic,
            &acceleration,
            &angularRate);

        ypr.yaw -= avgYpr.yaw;
        ypr.pitch -= avgYpr.pitch;
        ypr.roll -= avgYpr.roll;        

        /* Print calibrated gravity compensated measurements. */
        ROS_INFO("IMU Solution:\n"
            "  Acceleration.x:         %+#7.2f\n"
            "  Acceleration.y:         %+#7.2f\n"
            "  Acceleration.z:         %+#7.2f\n",
            acceleration.c0,
            acceleration.c1,
            acceleration.c2);
        ROS_INFO("IMU Solution:\n"
            "  Angular velocity.x:     %+#7.2f\n"
            "  Angular velocity.y:     %+#7.2f\n"
            "  Angular velocity.z:     %+#7.2f\n",
            angularRate.c0,
            angularRate.c1,
            angularRate.c2);
        ROS_INFO("IMU Solution:\n"
            "  Yaw:                    %+#7.2f\n"
            "  Pitch:                  %+#7.2f\n"
            "  Roll:                   %+#7.2f\n",
            ypr.yaw,
            ypr.pitch,
            ypr.roll);
	

        if (errorCode != VNERR_NO_ERROR){
            ROS_FATAL("Error encountered.\n");
            exit(0);
        }

        imu_msg.header.seq = sequence_id++;
        imu_msg.header.stamp = ros::Time::now();
        imu_msg.header.frame_id = "imu/data";

        //To do: verify these expressions if they give the right quaternions
        float halfYaw = float(ypr.yaw) * float(0.5);  
        float halfPitch = float(ypr.pitch) * float(0.5);  
        float halfRoll = float(ypr.roll) * float(0.5);  
        float cosYaw = cosf(halfYaw);
        float sinYaw = sinf(halfYaw);
        float cosPitch = cosf(halfPitch);
        float sinPitch = sinf(halfPitch);
        float cosRoll = cosf(halfRoll);
        float sinRoll = sinf(halfRoll);

        imu_msg.orientation.x = ( cosRoll * sinPitch * cosYaw + sinRoll * cosPitch * sinYaw );
        imu_msg.orientation.y = ( cosRoll * cosPitch * sinYaw - sinRoll * sinPitch * cosYaw );
        imu_msg.orientation.z = ( sinRoll * cosPitch * cosYaw - cosRoll * sinPitch * sinYaw );
        imu_msg.orientation.w = ( cosRoll * cosPitch * cosYaw + sinRoll * sinPitch * sinYaw );

        //To do: Calculate its covariance matrix
        imu_msg.angular_velocity.x = angularRate.c0;
        imu_msg.angular_velocity.y = angularRate.c1;
        imu_msg.angular_velocity.z = angularRate.c2;

        //To do: Calculate its covariance matrix
        imu_msg.linear_acceleration.x = acceleration.c0;
        imu_msg.linear_acceleration.y = acceleration.c1;
        imu_msg.linear_acceleration.z = acceleration.c2;

        return imu_msg;
    }
}
















/*
	VN_ERROR_CODE errorCode;
	double gpsTime;
	unsigned short gpsWeek, status;
	VnVector3 ypr1, latitudeLognitudeAltitude, nedVelocity;
	float attitudeUncertainty, positionUncertainty, velocityUncertainty;
	Vn200 vN200;
	int i;
	
	vn200_getInsSolutionLla(
	    &vN200,
	    &gpsTime,
	    &gpsWeek,
	    &status,
	    &ypr1,
	    &latitudeLognitudeAltitude,
	    &nedVelocity,
	    &attitudeUncertainty,
	    &positionUncertainty,
	    &velocityUncertainty);
	printf("INS Solution:\n"
	    "  GPS Time:               %f\n"
	    "  GPS Week:               %u\n"
	    "  INS Status:             %.4X\n"
	    "  YPR.Yaw:                %+#7.2f\n"
	    "  YPR.Pitch:              %+#7.2f\n"
	    "  YPR.Roll:               %+#7.2f\n"
	    "  LLA.Latitude:           %+#7.2f\n"
	    "  LLA.Longitude:          %+#7.2f\n"
	    "  LLA.Altitude:           %+#7.2f\n"
	    "  Velocity.North:         %+#7.2f\n"
	    "  Velocity.East:          %+#7.2f\n"
	    "  Velocity.Down:          %+#7.2f\n"
	    "  Attitude Uncertainty:   %+#7.2f\n"
	    "  Position Uncertainty:   %+#7.2f\n"
	    "  Velocity Uncertainty:   %+#7.2f\n",
	    gpsTime,
	    gpsWeek,
	    status,
	    ypr1.c0,
	    ypr1.c1,
	    ypr1.c2,
	    latitudeLognitudeAltitude.c0,
	    latitudeLognitudeAltitude.c1,
	    latitudeLognitudeAltitude.c2,
	    nedVelocity.c0,
	    nedVelocity.c1,
	    nedVelocity.c2,
	    attitudeUncertainty,
	    positionUncertainty,
	    velocityUncertainty);
	printf("\n\n");
	sleep(1);	
*/
