// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Annot.cc
// Segment 76/79



AnnotRichMedia::Deactivation::Condition AnnotRichMedia::Deactivation::getCondition() const
{
    return condition;
}

AnnotRichMedia::Content::Content(Dict *dict)
{
    Object obj1 = dict->lookup("Configurations");
    if (obj1.isArray()) {
        int numberOfConfigurations = obj1.arrayGetLength();
        configurations.reserve(numberOfConfigurations);

        for (int i = 0; i < numberOfConfigurations; ++i) {
            Object obj2 = obj1.arrayGet(i);
            if (obj2.isDict()) {
                configurations.push_back(std::make_unique<AnnotRichMedia::Configuration>(obj2.getDict()));
            } else {
                configurations.emplace_back();
            }
        }
    }

    obj1 = dict->lookup("Assets");
    if (obj1.isDict()) {
        Object obj2 = obj1.getDict()->lookup("Names");
        if (obj2.isArray()) {
            const int length = obj2.arrayGetLength() / 2;
            assets.reserve(length);

            for (int i = 0; i < length; ++i) {
                Object objKey = obj2.arrayGet(2 * i);
                Object objVal = obj2.arrayGet(2 * i + 1);

                if (!objKey.isString() || objVal.isNull()) {
                    error(errSyntaxError, -1, "Bad Annot Asset");
                    continue;
                }

                auto asset = std::make_unique<AnnotRichMedia::Asset>();

                asset->name = objKey.takeString();
                asset->fileSpec = std::move(objVal);
                assets.push_back(std::move(asset));
            }
        }
    }
    assets.shrink_to_fit();
}

AnnotRichMedia::Content::~Content() = default;

AnnotRichMedia::Asset::Asset() = default;

AnnotRichMedia::Asset::~Asset() = default;

const GooString *AnnotRichMedia::Asset::getName() const
{
    return name.get();
}

Object *AnnotRichMedia::Asset::getFileSpec() const
{
    return const_cast<Object *>(&fileSpec);
}

AnnotRichMedia::Configuration::Configuration(Dict *dict)
{
    Object obj1 = dict->lookup("Instances");
    if (obj1.isArray()) {
        int numberOfInstances = obj1.arrayGetLength();
        instances.reserve(numberOfInstances);

        for (int i = 0; i < numberOfInstances; ++i) {
            Object obj2 = obj1.arrayGet(i);
            if (obj2.isDict()) {
                instances.push_back(std::make_unique<AnnotRichMedia::Instance>(obj2.getDict()));
            } else {
                instances.emplace_back();
            }
        }
    }

    obj1 = dict->lookup("Name");
    if (obj1.isString()) {
        name = obj1.takeString();
    }

    obj1 = dict->lookup("Subtype");
    if (obj1.isName()) {
        const char *subtypeName = obj1.getName();

        if (!strcmp(subtypeName, "3D")) {
            type = type3D;
        } else if (!strcmp(subtypeName, "Flash")) {
            type = typeFlash;
        } else if (!strcmp(subtypeName, "Sound")) {
            type = typeSound;
        } else if (!strcmp(subtypeName, "Video")) {
            type = typeVideo;
        } else {
            // determine from first non null instance
            type = typeFlash; // default in case all instances are null
            for (auto &instance : instances) {
                if (instance) {
                    switch (instance->getType()) {
                    case AnnotRichMedia::Instance::type3D:
                        type = type3D;
                        break;
                    case AnnotRichMedia::Instance::typeFlash:
                        type = typeFlash;
                        break;
                    case AnnotRichMedia::Instance::typeSound:
                        type = typeSound;
                        break;
                    case AnnotRichMedia::Instance::typeVideo:
                        type = typeVideo;
                        break;
                    }
                    // break the loop since we found the first non null instance
                    break;
                }
            }
        }
    }
}

AnnotRichMedia::Configuration::~Configuration() = default;

const GooString *AnnotRichMedia::Configuration::getName() const
{
    return name.get();
}

AnnotRichMedia::Configuration::Type AnnotRichMedia::Configuration::getType() const
{
    return type;
}

AnnotRichMedia::Instance::Instance(Dict *dict)
{
    Object obj1 = dict->lookup("Subtype");
    const char *name = obj1.isName() ? obj1.getName() : "";

    if (!strcmp(name, "3D")) {
        type = type3D;
    } else if (!strcmp(name, "Flash")) {
        type = typeFlash;
    } else if (!strcmp(name, "Sound")) {
        type = typeSound;
    } else if (!strcmp(name, "Video")) {
        type = typeVideo;
    } else {
        type = typeFlash;
    }

    obj1 = dict->lookup("Params");
    if (obj1.isDict()) {
        params = std::make_unique<AnnotRichMedia::Params>(obj1.getDict());
    }
}

AnnotRichMedia::Instance::~Instance() = default;

AnnotRichMedia::Instance::Type AnnotRichMedia::Instance::getType() const
{
    return type;
}