VR traffic model for ns-3
=========================

**NOTE:** You can find detailed information about this module in the referenced papers below.

Examples of usage for the newly introduced features can be found in the ``examples/`` folder.

## Features

This module comprises the following features:

* Adds `BurstyApplication` and `BurstSink` as ns-3 applications: they allow to model complex applications by sending large data packets over UDP sockets, fragmenting them into bursts of smaller packets, and reassembling the packets at the receiver, if possible
* A tracing system allows to obtain burst-level and fragment-level information at both the transmitter and receiver side
* Models Virtual Reality traffic sources with realistic head movements in popular VR applications
* 40 of the acquired VR traffic traces can be found in BurstGeneratorTraces/ and can be used directly in a simulation, using the `TraceFileBurstGenerator`. More information can be found in the folder and in the documentation.
* Additional traffic models can be implemented by simply extending the `BurstGenerator` interface

Future releases will aim to:
* Model second-order statistics for VR traffic
* Include a head-rotation model, coupled with the VR traffic generator
* Optionally include sound traffic in the VR traffic generator, other than video traffic
* Optionally include uplink/downlink control packets for the VR traffic model
* Improve `BurstSink` to also include some form of forward error correction

More information can be found in the reference paper(s).

## Install

### Prerequisites ###

To run simulations using this module, you will need to install ns-3, and clone
this repository inside the `contrib` directory.
Required dependencies include git and a build environment.

#### Installing dependencies ####

Please refer to [the ns-3 wiki](https://www.nsnam.org/wiki/Installation) for instructions on how to set up your system to install ns-3.

Please check the [releases](https://github.com/signetlabdei/ns-3-vr-app/releases) for further information about dependencies.

#### Downloading #####

First, clone the main ns-3 repository:

```bash
git clone https://gitlab.com/nsnam/ns-3-dev ns-3-dev
```

Then, clone the vr-app module:
```bash
git clone https://github.com/signetlabdei/ns-3-vr-app ns-3-dev/contrib/vr-app
```

### Compilation ###

Configure and build ns-3 from the `ns-3-dev` folder:

```bash
./ns3 configure --enable-tests --enable-examples
./ns3 build
```

This module does not provide Python bindings at the moment.

### Documentation ###

To compile the documentation, please follow the instructions from the [ns-3 manual](https://www.nsnam.org/docs/manual/html/documentation.html).

Basic steps:

1. Install the documentation-specific dependencies as described in the [ns-3 installation guide](https://www.nsnam.org/wiki/Installation)
1. You might need to fix the ImageMagick permissions for ghostscript files

Compiling standalone documentation:

1. Run in your terminal, in the ns-3 root folder, `make -C contrib/vr-app/doc html`
1. Open with your browser the file `contrib/vr-app/doc/models/build/html/bursty-framework.html` to visualize the documentation of the model


## Reference papers

* M. Lecci, M. Drago, A. Zanella, M. Zorzi, "An Open Framework for Analyzing and Modeling XR Network Traffic," in IEEE Access, vol. 9, pp. 129782-129795, 2021. Open Access DOI: [10.1109/ACCESS.2021.3113162](https://doi.org/10.1109/ACCESS.2021.3113162).
* M. Lecci, A. Zanella, M. Zorzi, "An ns-3 Implementation of a Bursty Traffic Framework for Virtual Reality Sources," Workshop on ns-3 (WNS3), Jun. 2021, Virtual Event, USA, Open access DOI: [10.1145/3460797.3460807](https://doi.org/10.1145/3460797.3460807).
