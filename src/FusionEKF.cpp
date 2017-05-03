#include "FusionEKF.h"
#include "tools.h"
#include "Eigen/Dense"
#include <iostream>

using namespace std;
using Eigen::MatrixXd;
using Eigen::VectorXd;
using std::vector;

/*
 * Constructor.
 */
FusionEKF::FusionEKF() {
    is_initialized_ = false;
    previous_timestamp_ = 0;

    // initializing matrices
    R_laser_ = MatrixXd(2, 2);
    R_radar_ = MatrixXd(3, 3);
    H_laser_ = MatrixXd(2, 4);
    Hj_ = MatrixXd(3, 4);

    //measurement covariance matrix - laser
    R_laser_ << 0.0225, 0,
            0, 0.0225;

    //measurement covariance matrix - radar
    R_radar_ << 0.09, 0, 0,
            0, 0.0009, 0,
            0, 0, 0.09;
    /**
    TODO:
      * Finish initializing the FusionEKF.
      * Set the process and measurement noises
    */
    //measurement matrix - laser
    H_laser_ << 1, 0, 0, 0,
            0, 1, 0, 0;

    //create a 4D state vector, we don't know yet the values of the x state
    VectorXd x_ = VectorXd(4);

    //state covariance matrix P
    MatrixXd P_ = MatrixXd(4, 4);
    P_ << 1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1000, 0,
            0, 0, 0, 1000;

    //the initial transition matrix F_
    MatrixXd F_ = MatrixXd(4, 4);
    F_ << 1, 0, 1, 0,
            0, 1, 0, 1,
            0, 0, 1, 0,
            0, 0, 0, 1;

    //measurement matrix
    MatrixXd H_ = MatrixXd();

    //measurement covariance
    MatrixXd R_ = MatrixXd();

    //set the process covariance matrix Q
    MatrixXd Q_ = MatrixXd(4, 4);

    //init initializes Kalman filter
    ekf_.Init(x_, P_, F_, H_, R_, Q_);
}

/**
* Destructor.
*/
FusionEKF::~FusionEKF() {}

void FusionEKF::ProcessMeasurement(const MeasurementPackage &measurement_pack) {


    /*****************************************************************************
     *  Initialization
     ****************************************************************************/
    if (!is_initialized_) {
        /**
        TODO:
          * Initialize the state ekf_.x_ with the first measurement.
          * Create the covariance matrix.
          * Remember: you'll need to convert radar from polar to cartesian coordinates.
        */
        // first measurement
//        cout << "EKF: " << endl;
        ekf_.x_ = VectorXd(4);
        ekf_.x_ << 1, 1, 1, 1;
        if (measurement_pack.sensor_type_ == MeasurementPackage::RADAR) {
            /**
            Convert radar from polar to cartesian coordinates and initialize state.
            */
            float ro = measurement_pack.raw_measurements_(0);
            float phi = measurement_pack.raw_measurements_(1);
            ekf_.x_(0) = ro * cos(phi);
            ekf_.x_(1) = ro * sin(phi);
        } else if (measurement_pack.sensor_type_ == MeasurementPackage::LASER) {
            /**
            Initialize state.
            */
            ekf_.x_ << measurement_pack.raw_measurements_[0], measurement_pack.raw_measurements_[1], 0, 0;
        }
        previous_timestamp_ = measurement_pack.timestamp_;

        // done initializing, no need to predict or update
        is_initialized_ = true;
        return;
    }

    /*****************************************************************************
     *  Prediction
     ****************************************************************************/

    /**
     TODO:
       * Update the state transition matrix F according to the new elapsed time.
        - Time is measured in seconds.
       * Update the process noise covariance matrix.
       * Use noise_ax = 9 and noise_ay = 9 for your Q matrix.
     */
    processCovariance(measurement_pack);

    ekf_.Predict();

    /*****************************************************************************
     *  Update
     ****************************************************************************/

    /**
     TODO:
       * Use the sensor type to perform the update step.
       * Update the state and covariance matrices.
     */

    if (measurement_pack.sensor_type_ == MeasurementPackage::RADAR) {
        // Radar updates
        Hj_ = tools.CalculateJacobian(ekf_.x_);
        ekf_.H_ = Hj_;
        ekf_.R_ = R_radar_;
        ekf_.UpdateEKF(measurement_pack.raw_measurements_);
    } else {
        // Laser updates
        ekf_.H_ = H_laser_;
        ekf_.R_ = R_laser_;
        ekf_.Update(measurement_pack.raw_measurements_);
    }

    // print the output
//    cout << "x_ = " << ekf_.x_ << endl;
//    cout << "P_ = " << ekf_.P_ << endl;
}
/**
   * Update the state transition matrix F according to the new elapsed time.
        - Time is measured in seconds.
   * Update the process noise covariance matrix.
   * Use noise_ax = 9 and noise_ay = 9 for your Q matrix.
 */
void FusionEKF::processCovariance(const MeasurementPackage &measurement_pack) {

    float dt = (measurement_pack.timestamp_ - previous_timestamp_) / 1000000.0;    // expressed in seconds
    previous_timestamp_ = measurement_pack.timestamp_;

    float dt_2 = dt * dt;
    float dt_3 = dt_2 * dt;
    float dt_4 = dt_3 * dt;

    //Modify the F matrix so that the time is integrated
    ekf_.F_(0, 2) = dt;
    ekf_.F_(1, 3) = dt;

    //set the process covariance matrix Q
    ekf_.Q_ = MatrixXd(4, 4);
    ekf_.Q_ << dt_4 / 4 * NOISE_AX, 0, dt_3 / 2 * NOISE_AX, 0,
            0, dt_4 / 4 * NOISE_AY, 0, dt_3 / 2 * NOISE_AY,
            dt_3 / 2 * NOISE_AX, 0, dt_2 * NOISE_AX, 0,
            0, dt_3 / 2 * NOISE_AY, 0, dt_2 * NOISE_AY;
}
