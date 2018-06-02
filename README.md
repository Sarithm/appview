# appview 

## Dynamic Application view
Build  application/infrastructure view 

### Monitor Application 
* Non-intrusive application monitor 
* Monitor application process state, not `just` **UP/DOWN**. But is it working as expected?
* List all service ports 
* Monitor number of active connections, build learning model and watch out for outliers.
* Montior application's servers connections such as database, messaging server etc.,

### Build application view.  
* Each active connection should be displayed in Green.
* Connections, which are down will be marked red, and dropped after certain time, for example after 168 hours.

### Monitor all running container on an instance, perhaps Kubernetes node
* List all container running on node and specs, such as application processes running in a container and service ports etc.,
* Container IP address, process name and owner uid etc.,
* Monitor number of active connections between continers,  build model and watch out for ourliers.
* Montior application's servers connections such as Database, DNS, proxy, messaging containers.
