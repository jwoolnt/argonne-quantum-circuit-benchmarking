from os import path, mkdir, remove, getcwd, chdir
from shutil import rmtree, copyfile

from intelqsdk.cbindings import compileProgram, loadSdk


COMPILER_PATH = "/opt/intel/quantum-sdk/latest/intel-quantum-compiler"
CIRCUITS_FOLDER = "src/circuits"
OUTPUT_FOLDER = "qbuild"
VISUALIZATION_FOLDER = "Visualization"
VISUALIZATION_OPTIONS = {"console", "tex", "json"}


def compileAndLoad(
		sdk_name: str,
		/,
		circuits_folder: str = CIRCUITS_FOLDER,
		output_folder: str = OUTPUT_FOLDER,
		visualization_folder: str = VISUALIZATION_FOLDER,
		*,
		clear_qbuild: bool = False,
		clear_visualizations: bool = True,
		replace: bool = True,
		copy_compile: bool = True,
		visualization: str = "json"
	):
	file_name = f"{sdk_name}.cpp"
	shared_object_name = f"{sdk_name}.so"
	file_path = path.join(circuits_folder, file_name)
	shared_object_path = path.join(output_folder, shared_object_name)

	def if_exists(check_path: str, function, *args, inverse: bool = False, **kwargs):
		if path.exists(check_path) ^ inverse:
			function(check_path, *args, **kwargs)

	if clear_qbuild:
		if_exists(output_folder, rmtree, ignore_errors=True)

	if clear_visualizations:
		if_exists(visualization_folder, rmtree, ignore_errors=True)

	if not replace and path.exists(shared_object_path):
		load(sdk_name, output_folder)
		return

	if_exists(output_folder, mkdir, inverse=True)
	if copy_compile:
		file_path = copyfile(file_path, path.join(output_folder, file_name))

	flags = {
		"o": output_folder,
		"P": visualization if visualization in VISUALIZATION_OPTIONS else None
		# TODO: Support multiple visualizations at once
	}
	flags = [f"-{key} {value}" for (key, value) in flags.items() if value]
	flags.append("-s")
	flags = " ".join(flags)

	compileProgram(COMPILER_PATH, file_path, flags, sdk_name)

	# TODO: Move latex files to visualization folder

	if copy_compile:
		remove(file_path)
	else:
		print("Ignore \"Failed to load program!\" warning above")
		load(sdk_name)

def load(sdk_name: str, /, output_folder: str = OUTPUT_FOLDER):
	shared_object_name = f"{sdk_name}.so"
	shared_object_path = path.join(output_folder, shared_object_name)
	loadSdk(shared_object_path, sdk_name)


if __name__ == "__main__":
	compileAndLoad("example", visualization="json")
