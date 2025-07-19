from intelqsdk.cbindings import CbitRef

def cbits_to_state(
		cbit_ref: list[CbitRef],
		n: int | None = None,
		/,
		label_states: bool = False
	) -> str:
	print_range = range(n if n is not None else len(cbit_ref))
	state = ["1" if cbit_ref[i].value() else "0" for i in print_range]
	state = "".join(state)
	state = f"|{state}>"
	return label_state(state) if label_states else state

def label_state(state: str) -> str:
	return state if state[1] == "0" else complement_state(state)

def complement_state(state: str) -> str:
	state = state[1:-1]
	state = ["1" if c == "0" else "0" for c in state]
	state = "".join(state)
	return f"|{state}>"


if __name__ == "__main__":
	for state in ["|0000>", "|1111>", "|0101>", "|1010>"]:
		print(state, complement_state(state), label_state(state))
