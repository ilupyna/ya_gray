## Final task from Yandex & MIPT C++ Coursera "Gray belt" (4/5 of specialization)
### Transport manager, which takes the data about stops and buses, and response for requests about descriptions and routes. Input/output in Json format (for this version).


General task description below

#### Given: 
- json parser in [``` json.h ```](json.h) w/o double and bool support, w/o print function;
- [``` graph.h ```](graph.h) & [``` router.h ```](router.h) for routes finding;
- json structure, patameters limits;
- time and memory limitations for this version: 3s, 512Mb for quantity 100 stops and buses each.


#### Needed:
- Take data about stops, routes, routing parameters from json formatted cin;
- Combine data and find routes;
- Response to inserted requests to json formatted cout.
