# 云台测试节点使用说明

## 1. 功能概述

云台测试节点用于在没有遥控器的情况下，为云台控制链路提供测试输入，方便调试云台角度控制和参数。

测试节点不直接发布 `gimbal_cmd`，而是发布 `gimbal_test_input` 测试输入消息。`RobotCMDTask()` 订阅测试输入，将其转换为正常的 `gimbal_cmd`，再由 `GimbalTask()` 订阅并执行。

因此，测试模式下的控制链路仍然是：

```text
GimbalTestTask()
    -> gimbal_test_input
    -> RobotCMDTask()
    -> gimbal_cmd
    -> GimbalTask()
    -> 云台电机
```

`RobotCMDTask()` 仍然是 `gimbal_cmd` 的唯一发布者。

## 2. 开启测试模式

打开：

```text
application/robot_def.h
```

找到：

```c
#define GIMBAL_TEST_MODE 0
```

改为：

```c
#define GIMBAL_TEST_MODE 1
```

开关含义：

- `0`：正常模式，使用原有遥控器、图传和视觉控制链路。
- `1`：测试模式，使用 `application/gimbal_test/` 中的测试节点作为云台控制输入。

修改后需要重新编译并烧录。

## 3. 修改固定目标位置

打开：

```text
application/gimbal_test/gimbal_test.c
```

修改以下两个参数：

```c
#define GIMBAL_TEST_YAW_OFFSET_DEG   0.0f
#define GIMBAL_TEST_PITCH_OFFSET_DEG 0.0f
```

例如：

```c
#define GIMBAL_TEST_YAW_OFFSET_DEG   5.0f
#define GIMBAL_TEST_PITCH_OFFSET_DEG -3.0f
```

表示云台进入测试模式后：

- yaw 移动到测试开始姿态的 `+5°`；
- pitch 移动到测试开始姿态的 `-3°`；
- 到达目标后保持。

这里的角度是相对偏移量，不是绝对 IMU 角度。

测试模式启动后，`RobotCMDTask()` 会等待收到云台反馈，然后记录当时的 yaw 和 pitch 作为基准姿态。最终目标为：

```text
目标 yaw   = 测试开始时 yaw   + yaw_offset_deg
目标 pitch = 测试开始时 pitch + pitch_offset_deg
```

## 4. 修改连续动作

固定目标由 `GimbalTestTask()` 每次发布。需要做连续运动时，可以在这个函数中根据时间修改测试输入。

### 4.1 yaw 正负往返

在 `gimbal_test.c` 中加入：

```c
#include "bsp_dwt.h"
```

然后将 `GimbalTestTask()` 修改为：

```c
void GimbalTestTask(void)
{
    static float start_ms = 0.0f;
    float elapsed_ms;

    if (start_ms == 0.0f)
        start_ms = DWT_GetTimeline_ms();

    elapsed_ms = DWT_GetTimeline_ms() - start_ms;

    if (elapsed_ms < 2000.0f)
    {
        gimbal_test_input.yaw_offset_deg = 10.0f;
        gimbal_test_input.pitch_offset_deg = 0.0f;
    }
    else if (elapsed_ms < 4000.0f)
    {
        gimbal_test_input.yaw_offset_deg = -10.0f;
        gimbal_test_input.pitch_offset_deg = 0.0f;
    }
    else
    {
        start_ms = DWT_GetTimeline_ms();
    }

    PubPushMessage(gimbal_test_pub, &gimbal_test_input);
}
```

动作顺序为：

```text
测试开始姿态 -> yaw +10° -> yaw -10° -> 重复
```

### 4.2 yaw 正弦运动

也可以使用正弦目标测试跟踪效果。加入：

```c
#include "bsp_dwt.h"
#include <math.h>
```

将 `GimbalTestTask()` 修改为：

```c
void GimbalTestTask(void)
{
    static float start_ms = 0.0f;
    float time_s;

    if (start_ms == 0.0f)
        start_ms = DWT_GetTimeline_ms();

    time_s = (DWT_GetTimeline_ms() - start_ms) * 0.001f;

    gimbal_test_input.yaw_offset_deg =
        5.0f * sinf(2.0f * 3.1415926f * 0.2f * time_s);
    gimbal_test_input.pitch_offset_deg = 0.0f;

    PubPushMessage(gimbal_test_pub, &gimbal_test_input);
}
```

这个示例表示：

- yaw 振幅为 `5°`；
- 频率为 `0.2 Hz`；
- pitch 保持测试开始姿态。

建议调试时先使用较小的振幅和较低的频率，确认方向、反馈和限位都正常后再逐步增加。

## 5. 测试模式下的行为

测试模式下，`RobotCMDTask()` 会执行以下处理：

- 不依赖 DJI 遥控器在线；
- 不依赖图传遥控器在线；
- 不进入原有遥控器丢控保护分支；
- 底盘控制量强制为零；
- 发射机构关闭；
- 等待测试输入和云台反馈都到达后，才进入云台闭环控制；
- pitch 目标继续受到现有软件限位约束。

当前 pitch 软件限位为：

```c
#define PITCH_MIN_LIMIT -20.0f
#define PITCH_MAX_LIMIT 30.0f
```

位于：

```text
application/robot_def.h
```

测试输入超过 pitch 限位时，会被限制在允许范围内。当前测试节点没有额外的 yaw 软件限位。

## 6. 编译和烧录

修改测试开关或测试动作后，需要重新编译固件并烧录。项目使用 CMake，可以执行：

```powershell
cmake --build --preset Debug
```

编译成功后，构建目录中会生成：

```text
build/Debug/basic_framework.elf
build/Debug/basic_framework.hex
build/Debug/basic_framework.bin
```

使用工程现有的烧录方式将对应固件烧录到云台板。

## 7. 测试结束后的恢复

测试完成后，必须将：

```c
#define GIMBAL_TEST_MODE 1
```

恢复为：

```c
#define GIMBAL_TEST_MODE 0
```

然后重新编译并烧录。否则下一次上电仍会进入测试模式，遥控器输入不会作为云台控制源。

## 8. 建议的调试顺序

建议按照以下顺序进行：

1. 先保持 yaw 和 pitch 偏移均为 `0°`，确认云台不会突然转动。
2. 将单轴偏移设置为 `2°~5°`，确认运动方向正确。
3. 先测试固定目标，再测试往返动作。
4. 调 PID 时使用较小幅度、较低频率的正弦目标。
5. 确认云台运行稳定后，再逐步增大角度、速度和频率。
6. 测试结束后关闭 `GIMBAL_TEST_MODE` 并重新烧录正常模式。

测试时应确保云台周围没有可能被运动部件碰撞的物体，并准备好断电或急停措施。
