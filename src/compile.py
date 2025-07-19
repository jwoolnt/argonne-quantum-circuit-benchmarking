from os import path, mkdir, remove
from shutil import rmtree, copyfile

from intelqsdk.cbindings import compileProgram, loadSdk


COMPILER_PATH = "/opt/intel/quantum-sdk/latest/intel-quantum-compiler"
SRC_PATH = path.dirname(path.realpath(__file__))
OUTPUT_FOLDER = "qbuild"
VISUALIZATION_FOLDER = path.join(SRC_PATH, "Visualization")
VISUALIZATION_OPTIONS = {"console", "tex", "json"}


def compileAndLoad(
		sdk_name: str,
		/,
		output_folder = OUTPUT_FOLDER,
		*,
		clear: bool = True,
		copy_compile: bool = True,
		visualization: str | None = None
	):
	file_name = f"{sdk_name}.cpp"
	file_path = path.join(SRC_PATH, file_name)
	output_path = path.join(SRC_PATH, output_folder)

	def if_exists(check_path: str, function, inverse: bool = False, *args, **kwargs):
		if path.exists(check_path) ^ inverse:
			function(check_path, *args, **kwargs)

	if clear:
		if_exists(output_path, rmtree, ignore_errors=True)
		if_exists(VISUALIZATION_FOLDER, rmtree, ignore_errors=True)

	if_exists(output_path, mkdir, True)
	if copy_compile:
		file_path = copyfile(file_path, path.join(output_path, file_name))

	flags = {
		"o": output_path,
		"P": visualization if visualization in VISUALIZATION_OPTIONS else None
	}
	flags = [f"-{key} {value}" for (key, value) in flags.items() if value]
	flags.append("-s")
	flags = " ".join(flags)

	compileProgram(COMPILER_PATH, file_path, flags, sdk_name)

	# TODO: use "local" Visualization folder?

	if copy_compile:
		remove(file_path)
	else:
		print("Ignore \"Failed to load program!\" warning above")
		loadSdk(path.join(output_path, f"{sdk_name}.so"), sdk_name)


if __name__ == "__main__":
	compileAndLoad("example", visualization="json")
