from os import path, mkdir, remove, getcwd, chdir
from shutil import rmtree, copyfile

from intelqsdk.cbindings import compileProgram, loadSdk


COMPILER_PATH = "/opt/intel/quantum-sdk/latest/intel-quantum-compiler"
SRC_PATH = path.dirname(path.realpath(__file__))
OUTPUT_FOLDER = "qbuild"
VISUALIZATION_FOLDER = "Visualization"
VISUALIZATION_OPTIONS = {"console", "tex", "json"}


def compileAndLoad(
		sdk_name: str,
		/,
		output_folder: str = OUTPUT_FOLDER,
		visualization_folder: str = VISUALIZATION_FOLDER,
		*,
		src: bool = True,
		clear_visualizations: bool = True,
		clear_qbuild: bool = False,
		replace: bool = True,
		copy_compile: bool = True,
		visualization: str | None = None # TODO: Support multiple visualizations at once
	):
	file_name = f"{sdk_name}.cpp"
	shared_object_name = f"{sdk_name}.so"
	file_path = file_name
	output_path = output_folder
	shared_object_path = path.join(output_folder, shared_object_name)
	visualization_path = visualization_folder
	working_path = getcwd()
	if src:
		file_path = path.join(SRC_PATH, file_name)
		output_path = path.join(SRC_PATH, output_folder)
		shared_object_path = path.join(output_path, shared_object_name)
		visualization_path = path.join(SRC_PATH, visualization_folder)

	def if_exists(check_path: str, function, inverse: bool = False, *args, **kwargs):
		if path.exists(check_path) ^ inverse:
			function(check_path, *args, **kwargs)

	if clear_visualizations:
		if_exists(visualization_path, rmtree, ignore_errors=True)

	if clear_qbuild:
		if_exists(output_path, rmtree, ignore_errors=True)

	if not replace and path.exists(shared_object_path):
		loadSdk(shared_object_path, sdk_name)
		return

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

	if src: chdir(SRC_PATH)
	compileProgram(COMPILER_PATH, file_path, flags, sdk_name)
	if src: chdir(working_path)

	if copy_compile:
		remove(file_path)
	else:
		print("Ignore \"Failed to load program!\" warning above")
		loadSdk(shared_object_path, sdk_name)

def load(sdk_name: str, /, src: bool = True):
	shared_object_name = f"{sdk_name}.so"
	shared_object_path = path.join(SRC_PATH, shared_object_name) if src else shared_object_name
	loadSdk(shared_object_path, sdk_name)


if __name__ == "__main__":
	compileAndLoad("example", visualization="json")
