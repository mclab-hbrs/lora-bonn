## Measurement Data
The data and the signal strengths of the received packets from all gateways can be found in [samples.csv](samples.csv). In addition, the path loss predictions of several models calculated by [Signal-Server](https://github.com/Cloud-RF/Signal-Server) is saved in [signalserver.csv](signalserver.csv). The lines there correspond to the samples file. Finally, the location of the gateways is saved in [gateways.csv](gateways.csv). 

## Analysis
An analysis of the measurements and the path loss models was done in the Jupyter Notebook [analyze.ipynb](analyze.ipynb). To run the notebook, a [Dockerfile](Dockerfile) is provided.

## License
The measurement data is licensed under the [Data license Germany – attribution – version 2.0](https://www.govdata.de/dl-de/by-2-0).
