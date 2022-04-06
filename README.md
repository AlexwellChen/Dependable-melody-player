# EDA423 Real-time Music Player

## The basic idea
This music player is based on the implementation of the EDA223 project, the main goal is to add the CAN network communication protocol to the original project, to achieve a reliable boards network. All the boards connected to the network can play music in a round robin way, and can exchange leadership between the boards (The committee). The network can also detect The network can also detect the loss of members, which is divided into Master Failure and Slave Failure (The watchdog).

## Establish the network
The network is built based on the following processes.

![Establish the network](Graph/Establish.jpg)

The functions related to Master and Slave state switching are implemented in an object called *Committee*, which will be used to maintain a state machine that is used to handle four possible scenarios in the current network: Network Leadership Initialization, Network Leadership Exchange, Master Failure, and Slave Failure.

![State machine](Graph/State_machine.jpg)

### initMyRank
In this step, we manually assign an Id to each board, either hard-coded in the code or entered via the keyboard.

### initBoardNum
In this step we need to create a method called initBoardNum, in which we will send some CAN messages according to a certain time convention and count the number of specific signals received over a period of time to determine the number of boards in the network.

![Detect boards](Graph/Detect.jpg)

We can see that between T=0s and T=2s we send a msgId 122 and receive several msgId 122. we specify that at T=0s we send a msgId 122 and initialize committee.boardNum with a value of 1. for each msgId 122 received until T=2s we add the boardNum by 1 for each msgId 122 received until T=2s.

At T=2s is when we send msgId 126 and load the boardNum of the current board in msg.buff. The meaning here is to send the information about the number of boards in the network received by the current board to the members of the current network. Then we can see that in the following 1s we receive some msgId 126, and in the msg.buff of these messages we can get the number of members observed by other boards, and we take the largest of these values (including our own boardNum). The point here is that we know that the network members cannot join the network at the same time, but in sequence. Therefore we use the number observed by the first board to join the network as the number of our network members.

After T=3s we agree that the process of detect is finished and the messages related to msgId 122 and msgId 126 are no longer processed.

Here we also need to add a lock to avoid that after T=2s we still receive the msgId 122 message and generate a response to it. But this problem may not be very significant in this project because we have a small number of boards, at most three, and just need to pay attention to the operation at startup time.

### initMode
In this step we start initializing the state machine. 

First all boards should have an initial state of **Init**, which means a new member of the network (or a Master waiting to hand over leadership).

When we send msgId 127, we enter the Waiting state, in which we need to create an array flag, the length of which is the number of current members. flag[myRank] is initialized to 1 (we need to acknowledge our leadership ourselves), and whenever we receive a msgId 124, we make flag[msg. nodeId] = 1 and after each receipt we need to check if the current array is all 1s.

If all of them are 1 then it proves that everyone acknowledges our leadership and we can go to Master and send msgId 123 to declare our leadership.

If it contains 0, then we continue to wait.

If we are in the Waiting state and receive msgId 127, then we enter the Compete state. In the Compete state we need to compare the size of msg.nodeId and myRank for msgId 127, and we specify leaderRank = max(myRank, msg.nodeId).

If myRank is less than msg.nodeId, we fail the competition, return to the Init state and send msgId 124 to acknowledge the other leader.

If the competition succeeds, we return the Waiting state and do not send any message.

All members in Init state enter Slave state after receiving msgId 123 and assign msg.nodeId to leaderRank.

At this point, the leadership initialization of the network is complete.