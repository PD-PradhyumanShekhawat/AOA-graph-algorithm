# Data Source and Attribution Record

## Primary source

National Aeronautics and Space Administration (NASA), NASA Open Data Portal, **HIRENASD coarse unstructured**.

Dataset page:
https://data.nasa.gov/dataset/hirenasd-coarse-unstructured

The NASA page describes the resource as an unstructured HIRENASD mesh of coarse size (approximately 5.7 million nodes and 14.4 million elements) for node-centered solvers.

## Original resource page

NASA DASHlink, resource 382, **HIRENASD coarse unstructured**:
https://c3.ndc.nasa.gov/dashlink/resources/382/

The resource page identifies Markus Ritter as the contributing author and lists the Tecplot ASCII file `HIRENASD_coarse_nc_volume.dat.gz`.

## File used

`HIRENASD_coarse_nc_volume.dat.gz`

Direct source:
https://c3.ndc.nasa.gov/dashlink/static/media/dataset/HIRENASD_coarse_nc_volume.dat.gz

The downloaded compressed source was approximately 273 MB. It was processed as a compressed stream; a separate one-gigabyte decompressed copy was not required for preprocessing.

## Attribution and originality

The NASA mesh is external source data and is explicitly attributed above. The following were produced for this assignment:

- global node mapping;
- the three adjacency definitions;
- edge generation and deduplication;
- CSR graph representation;
- execution program;
- runtime and memory measurements;
- comparison and interpretation.

The NASA Open Data Portal page does not specify a dataset license. No claim of public-domain status is made here.
