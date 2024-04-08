#pragma once

#define START_COROUTINE(INIT_STATE_NAME) \
switch (co) { \
	case CoroutineState::INIT_STATE_NAME: {

#define END_COROUTINE() \
} \
case CoroutineState::Failure: \
	return false; \
}

#define YIELD(STATE_NAME) \
	co = CoroutineState::STATE_NAME; \
	return true; \
} \
case CoroutineState::STATE_NAME: {

#define YIELD_UNTIL(PAUSE_STATE_NAME, WAITED_STATE_NAME) \
	co = CoroutineState::PAUSE_STATE_NAME; \
	return true; \
} \
case CoroutineState::PAUSE_STATE_NAME: \
	return true; \
case CoroutineState::WAITED_STATE_NAME: {
