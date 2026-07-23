#ifndef GIMBAL_TEST_H
#define GIMBAL_TEST_H

#define GIMBAL_TEST_INPUT_TOPIC "gimbal_test_input"

typedef struct
{
    float yaw_offset_deg;
    float pitch_offset_deg;
} Gimbal_Test_Input_s;

void GimbalTestInit(void);
void GimbalTestTask(void);

#endif // GIMBAL_TEST_H
