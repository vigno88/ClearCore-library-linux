/*
 * Copyright (c) 2020 Teknic, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
    \file
    ClearCore Step & Direction motion profile generator

    A StepGenerator is activated by creating an instance of the StepGenerator
    class. There can be several instances of StepGenerator, however, each must
    be attached to different connectors.
**/

#ifndef __STEPGENERATOR_H__
#define __STEPGENERATOR_H__

#include <stdint.h>

namespace ClearCore {

/** All of the motor position, velocity, and acceleration parameters are signed
    and in Q format, with all arithmetic performed in fixed point.
    This defines the Q value - the number of bits that are treated as
    fractional values (15). **/
#define FRACT_BITS 15

/**
    \class StepGenerator
    \brief ClearCore motor motion generator class

    This class manages the generation and communication of movement profiles for
    the MotorDriver connectors.

    For more detailed information on the ClearCore Motor Control and Motion
    Generation systems, check out the \ref MotorDriverMain and \ref MoveGen
    informational pages.
**/
class StepGenerator {
    friend class MotorManager;
    friend class TestIO;

public:
#ifndef HIDE_FROM_DOXYGEN
    StepGenerator();
#endif

    /**
        \brief Issues a positional move for the specified distance.
        
        \note When making absolute moves, ClearCore tracks the current position
        based on the zero position at program-start.

        \code{.cpp}
        // Perform a 5000 step pulse absolute move
        ConnectorM0.Move(5000, true);
        \endcode

        \param[in] dist The distance of the move in step pulses
        \param[in] absolute (optional) True if the dist argument is an absolute
        position. Default: false.
    **/
    bool Move(int32_t dist, bool absolute = false);

    /**
        \brief Issues a velocity move at the specified velocity.

        \code{.cpp}
        // Perform a velocity move
        ConnectorM0.MoveVelocity(500);
        \endcode

        \note Any existing move will be immediately overwritten with the new
        velocity.

        \param[in] velocity The velocity of the move in step pulses/second.
    **/
    bool MoveVelocity(int32_t velocity);

    /**
        Interrupts the current move; the motor may stop abruptly.

        \code{.cpp}
        // Command an abrupt stop
        ConnectorM0.MoveStopAbrupt();
        \endcode
    **/
    void MoveStopAbrupt();

    /**
        \brief Sets the absolute commanded position to the given value.

        \code{.cpp}
        // Set the StepGenerator's position reference to -5000
        ConnectorM0.PositionRefSet(-5000);
        \endcode

        \param[in] posn The new position to be set.
    **/
    void PositionRefSet(int32_t posn) {
        m_posnAbsolute = posn;
    }

    /**
        \brief Accessor for the StepGenerator's position reference

        \code{.cpp}
        if (ConnectorM0.PositionRefCommanded() > 5000) {
            // M-0's position reference is above 5000
        }
        \endcode

        \return Returns the absolute commanded position.
    **/
    volatile const int32_t &PositionRefCommanded() {
        return m_posnAbsolute;
    }

    /**
        \brief Sets the maximum velocity in step pulses per second.

        Value will be clipped if out of bounds

        \code{.cpp}
        // Set the StepGenerator's maximum velocity to 1200 step pulses/sec
        ConnectorM0.VelMax(1200);
        \endcode

        \param[in] velMax The new velocity limit
    **/
    void VelMax(int32_t velMax);

    /**
        \brief Sets the maximum acceleration in step pulses per second^2.

        Value will be clipped if out of bounds

        \code{.cpp}
        // Set the StepGenerator's maximum velocity to 15000 step pulses/sec^2
        ConnectorM0.AccelMax(15000);
        \endcode

        \param[in] accelMax The new acceleration limit
    **/
    void AccelMax(int32_t accelMax);

    /**
        \brief Function to check if no steps are currently being commanded to
        the motor.

        \code{.cpp}
        if (ConnectorM0.StepsComplete()) {
            // No more steps are being commanded
        }
        \endcode

        \return Returns true if there is no valid current command.
        \note The motor may still be moving after steps are done being sent.
    **/
    bool StepsComplete() {
        return MoveStateGet() == MS_IDLE;
    }

protected:
    typedef enum {
        MS_IDLE,
        MS_START,
        MS_ACCEL,
        MS_CRUISE,
        MS_DECEL,
        MS_DECEL_VEL,
        MS_END,
    } MOVE_STATES;

    uint32_t m_stepsPrevious;
    uint32_t m_stepsPerSampleMax;
    MOVE_STATES m_moveState;
    bool m_direction;
    bool m_directionLast;

    volatile const bool &Direction() {
        return m_direction;
    }

    volatile const MOVE_STATES &MoveStateGet() {
        return m_moveState;
    }

    void StepsCalculated();

    uint32_t StepsPrevious() {
        return m_stepsPrevious;
    }

private:
    int32_t m_posnAbsolute;

    int32_t m_stepsCommanded;
    int32_t m_stepsSent;      // Accumulated integer position

    bool m_velocityMove;       // A Velocity move is active
    bool m_velMoveDirChange;   // The velocity move is changing direction

    // All of the position, velocity and acceleration parameters are signed and
    // in Q format, with all arithmetic performed in fixed point.
    // FRACT_BITS defines the Q value - the number of bits that are treated as
    // fractional values.

    int32_t m_velLimitQx;     // Velocity limit
    int32_t m_altVelLimitQx;  // Velocity move Velocity limit
    int32_t m_accelLimitQx;   // Acceleration limit
    int64_t m_posnCurrentQx;  // Current position
    int32_t m_velCurrentQx;   // Current velocity
    int32_t m_accelCurrentQx; // Current acceleration
    int64_t m_posnTargetQx;   // Move length
    int32_t m_velTargetQx;    // Adjusted velocity limit
    int64_t m_posnDecelQx;    // Position to start decelerating

    virtual void OutputDirection() = 0;
    void StepsPerSampleMaxSet(uint32_t maxSteps);

    void AltVelMax(int32_t velMax);
};

} // ClearCore namespace

#endif // __STEPGENERATOR_H__