#appview  - non-intrusive application monitor

## Dynamic Application view
Build  application/infrastructure view 

### Monitor Application
* Application process state, not just UP/DOWN but is it working as expected?
* List all service ports 
* Monitor number of active connections, build model and watch out for outliers.
* Montior application's server(s) connections such as Database, messaging server etc.,

### Build applcation view.  
* Each active connection should be displayed in Green
* Connections, which are down will be marked red, and dropped after certain time, for example after 168 hours 

### Monitor all running container on an instance, perhaps Kubernetes node
* List all container running on node and specs. Such as application processes running in container and service ports etc.,
* Container IP address
* Monitor number of active connections between continers,  build model and watch out for ourliers.
* Montior application's server(s) connections such as Database.
