import argparse
import zipfile
import os
import json
from pathlib import Path
import xml.etree.ElementTree as ET


def get_model_identifier(content):
    # Open modelDescription.xml from the zip archive

    # Parse the XML content
    tree = ET.parse(content)
    root = tree.getroot()

    # Find the CoSimulation element and get the modelIdentifier attribute
    co_simulation_element = root.find('.//CoSimulation')
    if co_simulation_element is not None:
        model_identifier = co_simulation_element.get('modelIdentifier')
        if model_identifier:
            print(f"Model Identifier: {model_identifier}")
            return model_identifier
        else:
            print("Error: 'modelIdentifier' attribute not found in CoSimulation.")
            return None
    else:
        print("Error: 'CoSimulation' element not found in the XML.")
        return None


def copy_and_replace_zip(input_zip, destination, connection_string, enable_statistics):
    output_zip = Path(destination) / Path(input_zip).name

    if output_zip == Path(input_zip):
        print("We are about to overwrite an existing zip file. STOP!")
        return

    # Open the original zip file
    with zipfile.ZipFile(input_zip, 'r') as zip_ref:

        if 'modelDescription.xml' in zip_ref.namelist():
            model_identifier = get_model_identifier(zip_ref.open('modelDescription.xml'))
        if model_identifier is None:
            print("Error no model identifier found")
            return

        # Create a new zip file to store the copied contents
        with zipfile.ZipFile(output_zip, 'w', zipfile.ZIP_DEFLATED) as new_zip:
            # Iterate over all the files in the original zip
            for file_info in zip_ref.infolist():
                # Skip the 'binaries' folder
                if 'binaries' in file_info.filename:
                    continue

                # Extract and write each file to the new zip
                new_zip.writestr(file_info, zip_ref.read(file_info.filename))

            new_zip.mkdir('resources')

            new_zip.writestr('resources/remote-config.json', json.dumps({
                'RemoteConnection': {'url': connection_string,
                                     'ShowStatistics': enable_statistics}
            }, indent=4))

            for p in Path('remote_fmu').glob('**/*'):
                rpath = str(Path(p).relative_to(Path('remote_fmu')))
                rpath = rpath.replace('remote_fmu', model_identifier)
                if p.is_dir():
                    new_zip.mkdir(str(rpath))
                else:
                    with open(p, 'rb') as f:
                        new_zip.writestr(str(rpath), f.read())


def main():
    # Create the argument parser
    parser = argparse.ArgumentParser(description="Process and modify a zip file with custom configurations.")

    parser.add_argument("input_zip", help="Path to the input zip file")
    parser.add_argument("--destination", help="Path to the destination directory", default=".")
    parser.add_argument("--connection_string", help="The remote connection string list localhost:50051",
                        default="localhost:50051")
    parser.add_argument("--statistics", action="store_true", help="Enable statistics on fmu free", default=True)

    args = parser.parse_args()

    # Ensure the script works on both Windows and Linux
    if not os.path.exists(args.input_zip):
        print(f"Error: The input zip file '{args.input_zip}' does not exist.")
        return

    if not os.path.isdir(args.destination):
        print(f"Error: The destination directory '{args.destination}' is invalid or does not exist.")
        return

    # Call the function to copy and replace zip
    copy_and_replace_zip(args.input_zip, args.destination, args.connection_string, args.statistics)


if __name__ == "__main__":
    main()
