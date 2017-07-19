# setup tensorflow

<https://www.tensorflow.org/install/install_linux>

> pip3 install --upgrade https://storage.googleapis.com/tensorflow/linux/cpu/tensorflow-1.2.0-cp35-cp35m-linux_x86_64.whl

# setup docker

<https://docs.docker.com/engine/installation/linux/ubuntu/>

# setup anaconda

<https://repo.continuum.io/archive/>

# must input code

> #!/usr/bin/env python

> import os

> os.environ['TF_CPP_MIN_LOG_LEVEL']='2' 

> # https://github.com/tensorflow/tensorflow/issues/7778

> import tensorflow as tf

# compile

> python3.5 -m py_compile [PYTHON FILE]
