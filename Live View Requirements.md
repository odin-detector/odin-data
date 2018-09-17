# Software Requirements

# Specification

#
## For

# Odin Data Live View Plugin

| **Document Information** | |
| --- |--- |
| Author(s) | Adam Neaves |
| Reviewer(s) |   |
| Version | 0.1 |
| Date | 17/09/2018 |

| **Version** | **Date** | **Author** | **Comments/Changes made** |
| --- | --- | --- | --- |
| 0.1 | 17/09/2018 | Adam Neaves | Created document |


# Introduction
## Product Purpose

The Live View Plugin is a plugin for Odin Data, that provides a downscaled set of the data on an external ZMQ Publisher Socket, and thus allows for external or internal software to access a live copy of the data to display to users, if need be. It can get a subset of the data either by only publishing 1 in N frames to the socket, or by displaying a certain number of frames per second. The behaviour of this plugin is fully configurable, in the same way the other plugins for Odin Data are.

This plugin does not provide a GUI that displays the data image, only reveals the data to a socket so that a separate peice of software may display it.

## References

The following list of documents were used during the creation of this plugin, or are ones this specification is directly related to.

- The ZeroMQ API reference
- The ZeroMQ guide book.
- [The Odin Workshop created by Tim Nicholls](https://github.com/stfc-aeg/odin-workshop)

## Design and Implementation Constraints

The plugin will be daisy chained in with the other plugins, and thus needs to be designed in a way that will not cause issue within this architecture. This means it should be able to process data frames at a reasonable speed to avoid bottlenecks within the system.

# Specification

## Configuration
The plugin can be configured in the same json file that the rest of the plugins are configured by. The following configuration options are available at this time:

- **live_view_socket_addr** 
  - A string that represents the address the live view images will be published to.
- **per_second** 
  - A bool, representing wether or not the plugin displays live view images in a 1 in every N frames manner, or N frames per second.
- **frame_frequency** 
  - an int that, depending on the *per_second* parameter, represents the N of the above parameter. So, if *per_second* is **true**, it represents the frames per second. If *per_second* is **false**, it instead represents how many frames need to pass through the plugin before it displays another.

This behaviour is currently subject to change depending on development of the plugin. Currently, it can only be set to display every nth frame, or display up to a number of frames per second, but cannot do both.

## Data Output

The data published on the ZMQ socket is a multipart message generated from the frame of data. It comes in two parts, a header and a data blob.

### Header
The header contains the following data as a json string
- frame_num - *int*
  - The number assigned to the frame. This is the number used to calculate if the frame should be shown when *per_second* is **false**.
- acquisition_id - *string*
  - The Acquisition ID number assigned by the detector. This can be blank.
- dtype - *string*
  - The data type of the following data blob. Represents the size of each pixel of data.
- dsize - *int*
  - The total size of the data blob in bytes
- compression *string*
  - The method of compression applied. Can be left as "unset"
- shape *int array*
  - An array describing the dimensions of the data. If plotted on a standard graph, shape[0] represents the x axis, and shape[1] the y axis.

### Data Blob
The second part of the two part message is the raw pixel data copied from the data frame. Using the information provided in the header, this can produce an image in a corresponding live image viewer. The data blob is an array of bytes, and must be manipulated to get it to a 2D array that can be represented as an image.

## ZMQ Socket
The socket interface provided is a 
[ZMQ Publish Socket](http://api.zeromq.org/2-1:zmq-socket#toc9), which allows multiple viewers to subscribe and receive the data. Adding extra viewers should not, according to the ZMQ specification, cause any additional load on the software, and each subscriber should get all the data published. Connection to a ZMQ socket can take a couple of seconds due to the underlying TCP connection protocol, which should be considered when designing a viewer GUI.