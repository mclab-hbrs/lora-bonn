FROM jupyter/scipy-notebook:d990a62010ae

RUN pip install -Iv geopy==2.1.0

# Jupyter Extentsions
RUN pip install -Iv jupyter_contrib_nbextensions==0.5.1 &&                    \
    jupyter contrib nbextension install --sys-prefix
RUN jupyter nbextension enable collapsible_headings/main --sys-prefix
RUN jupyter nbextension enable toc2/main --sys-prefix
RUN conda config --add channels conda-forge
RUN conda install ipyleaflet
RUN conda install qgrid

WORKDIR /home/jovyan/work

