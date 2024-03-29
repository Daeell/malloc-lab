## 진행 상황
1. `Implicit` - `first-fit`, `next-fit`구현
2.  `Explicit` - `LIFO` 구현 -->> `mm.c` 제출

## **What is Important in Dynamic Memory Allocator**

- `Dynamic memory allocator`의 성능의 지표는 `memory ultilization(메모리 이용도)`, `throughout(처리량)`이다.
- `memory utilization` : `heap segment`를 낭비하는 공간 없이 즉 `fragmentation(단편화)`를 최소화하여 필요한 데이터를 최대한 저장(`write`)할 수 있는가이다.
- `throughout(처리량)` : 얼마나 빠르게 `application`의 `malloc`, `free`, `realloc` 요청을 처리할 수 있는가의 문제이다.
- 두 지표는 `trade-off`, 상충하는 관계이므로 적절한 균형을 맞추어야 한다.
- `implicit`방식을 예로 들면 `throughout`을 높이기 위해서 header와 `footer`에 `size`와 `allocated`와 같은 정보를 담는다. 그리고 
이를 활용하여 `application`이 빠르게 데이터를 처리한다.
    반면 이러한 `overhead` 데이터는 꼭 필요한 정보가 아니기 때문에 낭비하는 공간에 해당되고, 이러한 점으로 인해 `memory utilization`이 낮아진다.

## About My Implmentation

- `LIFO`(선입선출)
    - 단순하고 상수시간에 처리됨. (`throughout` 향상)
    - `fragmentation`이 `address-ordered` policy보다 안 좋음.

- **Organization**
    - 할당이 해제되어 있는 블록들은 연결리스트 형태로 관리하는 방식이다.
    - `free block`내에 `prev`와 `next`포인터를 포함하는 이중연결리스트로 구현할 수 있다. 이러한 구조 때문에 `Explicit`방식에서는 각 `free block`의 `size` 정보뿐만 아니라 이전과 다음 `free block`에 대한 정보를 명시적으로 가지고 있다.
    - 모든 `block`을 검색하는 `implicit`방식을 개선하여, 검색할 때 모든 블록을 검사하지 않고, 할당이 해제된 블록들만 검사하는 방식이다. (실제 성능은 `implicit`에서 `next-fit`방식과 유사)
    - `free block`들은 논리적으로는 이중연결 리스트로 관리되지만, 실제로는 `free block`과 `alloc block`이 함께 존재하고, `free block`끼리만 `prev`와 `next`포인터로 연결되어 있는 것이다.
- 또한 연결리스트의 마지막 블록(`prologue block`)은 `allocated block`으로 설정하였음.

- **Allocating Blocks**
    - 연결리스트를 순회하면서 `free block`의 `size`를  확인한다.
    -  `block size`가 할당 `size`보다 작다면 해당 블록의 `next`포인터로 이동하여 다음 블록을 순회한다.
    -  `block size`가 할당하려는 `size`보다 크거나 같다면 할당한다.

          - 해당 `block`을 `free list`에서 제외한다.
          - `block size`가 할당할 수 있는 사이즈 공간보다 크다면, 블록을 분할하여 할당한다. 나머지 블록은 `free block`으로 변경한 후 
연결리스트에 추가한다. 할당 `size`가 정렬 기준 사이즈 공간보다 작다면 `padding`을 통해 해당 `block`을 정렬기준에 맞춘다.
          - 메모리를 할당하고 해당 `block`의 주소를 반환한다.
          - 할당할 수 있는 공간이 없다면 (연결리스트의 끝에 도달했다면) 힙을 확장한다.
  - 제거된 `block`의 `prev`와 `next`를 수정하여 `free list`의 앞 뒤 block을 연결한다.

- **Free Blocks**
  - 할당을 해제할 `block`의 주소를 받는다. 
  - 해제할 `block`의 앞 뒤의 `block`을 확인한다.
      - `free block`이 없다면 해당 `block`만 연결리스트의 가장 앞에 추가하고 `prev`와 `next`를 설정한다. `case1`
  - 해제할 `block`의 앞 혹은 뒤에 이미 `free block`이 있다면 해당 `free block`을 `free list`에서 제외한다. `case2-4` : `removeblock`
  - 해제할 block과 앞에서 제외한 free block과 합쳐 하나의 free block으로 만들고 연결리스트의 가장 앞에 추가한다. : `putblock`
  
- **ETC**
  - 32bit 환경에서 double word 정렬을 하기 위해 인접한 8의 배수의 크기로 block을 할당시켜야 함. 이를 위해서 기존에 선언되어있던 매크로인 `ALIGN`과 `SIZE_T_SIZE`를 활용함. `136행 참고`
  - `Explicit` 방식에서는 `free block`들간의 연결상태를 확인하고 조작하는 과정이 자주 발생함. 이를 위해 `PREV_FREE`, `NEXT_FREE`를 선언하여 `free block`들 간 논리적 연결관계를 조정하는데 활용함.
  - `realloc`에서 활용되는 `memcpy` 함수는 메모리의 값을 복사하는 기능을 가진 함수임. 첫번째 인자는 복사 받을 메모리를 가리키는 포인터, 두번째 인자는 복사할 메모리를 가리키고 있는 포인터. 세번째 인자는 복사할 데이터(값)의 길이임

#####################################################################
# CS:APP Malloc Lab
# Handout files for students
#
# Copyright (c) 2002, R. Bryant and D. O'Hallaron, All rights reserved.
# May not be used, modified, or copied without permission.
#
######################################################################
    
***********
Main Files:
***********

mm.{c,h}	
	Your solution malloc package. mm.c is the file that you
	will be handing in, and is the only file you should modify.

mdriver.c	
	The malloc driver that tests your mm.c file

short{1,2}-bal.rep
	Two tiny tracefiles to help you get started. 

Makefile	
	Builds the driver

**********************************
Other support files for the driver
**********************************

config.h	Configures the malloc lab driver
fsecs.{c,h}	Wrapper function for the different timer packages
clock.{c,h}	Routines for accessing the Pentium and Alpha cycle counters
fcyc.{c,h}	Timer functions based on cycle counters
ftimer.{c,h}	Timer functions based on interval timers and gettimeofday()
memlib.{c,h}	Models the heap and sbrk function

*******************************
Building and running the driver
*******************************
To build the driver, type "make" to the shell.

To run the driver on a tiny test trace:

	unix> mdriver -V -f short1-bal.rep

The -V option prints out helpful tracing and summary information.

To get a list of the driver flags:

	unix> mdriver -h

