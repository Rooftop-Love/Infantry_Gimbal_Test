#include "robot_def.h"

#if GIMBAL_TEST_MODE && defined(GIMBAL_BOARD)

#include "gimbal_test.h"
#include "message_center.h"

/* Test targets are offsets from the pose captured when test mode starts. */
#define GIMBAL_TEST_YAW_OFFSET_DEG   0.0f
#define GIMBAL_TEST_PITCH_OFFSET_DEG 0.0f

static Publisher_t *gimbal_test_pub;
static Gimbal_Test_Input_s gimbal_test_input = {
    .yaw_offset_deg = GIMBAL_TEST_YAW_OFFSET_DEG,
    .pitch_offset_deg = GIMBAL_TEST_PITCH_OFFSET_DEG,
};

void GimbalTestInit(void)
{
    gimbal_test_pub = PubRegister(GIMBAL_TEST_INPUT_TOPIC, sizeof(Gimbal_Test_Input_s));
}

void GimbalTestTask(void)
{
    PubPushMessage(gimbal_test_pub, &gimbal_test_input);
}

#endif
