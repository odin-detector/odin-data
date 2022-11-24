# Specification For Odin Data Live View Plugin

| **Document Information** |               |
| ------------------------ | ------------- |
| Author(s)                | Ashley Neaves |
| Reviewer(s)              |               |
| Version                  | 1.0           |
| Date                     | 18/09/2018    |

| **Version** | **Date**   | **Author**    | **Comments/Changes made**                            |
| ----------- | ---------- | ------------- | ---------------------------------------------------- |
| 1.0         | 18/09/2018 | Ashley Neaves | Updated to change from requirements to specification |
| 0.1         | 17/09/2018 | Ashley Neaves | Created document                                     |


## Introduction

The Live View Plugin is a plugin for Odin Data, that provides a downscaled set of the data on an external ZMQ Publisher Socket, and thus allows for external or internal software to access a live copy of the data to display to users, if need be. It can get a subset of the data either by only publishing 1 in N frames to the socket, or by displaying a certain number of frames per second. The behaviour of this plugin is fully configurable, in the same way the other plugins for Odin Data are.

This plugin does not provide a GUI that displays the data image, only provides an interface so that a separate piece of software may display it.

### References

The following list of documents were used during the creation of this plugin, or are ones this specification is directly related to.

- The ZeroMQ API reference
- The ZeroMQ guide book.
- [The Odin Workshop created by Tim Nicholls](https://github.com/stfc-aeg/odin-workshop)

## Specification

### Configuration
The plugin can be configured in the same json file that the rest of the plugins are configured by. The following configuration options are available at this time:

- **live_view_socket_addr**
  - A string that specifies the socket port and address the live view images will be published to.
- **per_second**
  - An int that specifies how many frames should be displayed each second. Setting this value to 0 will disable the *per Second* option, so the plugin will ignore the time between frames shown.
- **frame_frequency**
  - an int that configures the plugin to display every N<sup>th</sup> frame. Setting this value to 0 will disable the frequency, so that the plugin ignores the frame number.
- **dataset_name**
  - A string, representing a whitelist of dataset names that will be displayed by the live view. Dataset names are separated by commas, and trimmed of surrounding whitespace.
  Setting this to an empty string will disable this option, so the dataset value will not be considered when choosing frames to display.

Currently, the plugin is designed so that, if both *per_second* and *frame_frequency* are set, the *per_second* option overrides the *frame_frequency*. This means that the plugin will display every N<sup>th</sup> frame as specified by the *frame_frequency*, unless the elapsed time between frames displayed gets larger than specified by *per_second*, in which case it displays the next frame no matter what.

If both *frame_frequency* and *per_second* are set to 0, no live view images will be pushed to the socket, as both methods of frame selection are disabled. If this occurs, the plugin will warn the user of this behaviour when configured, but otherwise remains loaded and a part of the data pipeline.

### Data Output

The data published on the ZMQ socket is a multipart message generated from the frame of data. It comes in two parts, a header and a data blob.

#### Header
The header contains the following data as a json string
- frame_num - *int*
  - The number assigned to the frame. This is the number used to calculate if the frame should be shown when *per_second* is **false**.
- acquisition_id - *string*
  - The Acquisition ID number assigned by the detector. This can be blank.
- dtype - *string*
  - The data type of the following data blob. Represents the size of each pixel of data.
  - Possible values are: "unknown", "uint8", "uint16", "uint32", "uint64", "float".
- dsize - *int*
  - The total size of the data blob in bytes
- compression *string*
  - The method of compression applied. Can be left as "unset"
  - Possible Values are: "none", "LZ4", "BSLZ4".
- shape *int array*
  - An array describing the dimensions of the data. If plotted on a standard graph, shape[0] represents the x axis, and shape[1] the y axis.

#### Data Blob
The second part of the two part message is the raw pixel data copied from the data frame. Using the information provided in the header, this can produce an image in a corresponding live image viewer. The data blob is an array of bytes, and must be manipulated to get it to a 2D array that can be represented as an image.

### ZMQ Socket
The socket interface provided is a
[ZMQ Publish Socket](http://api.zeromq.org/2-1:zmq-socket#toc9), which allows multiple viewers to subscribe and receive the data. Adding extra viewers should not, according to the ZMQ specification, cause any additional load on the software, and each subscriber should get all the data published. Connection to a ZMQ socket can take a couple of seconds due to the underlying TCP connection protocol, which should be considered when designing a viewer GUI.
